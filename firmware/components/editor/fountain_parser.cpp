#include <cstring>
#include <cctype>

#include "fountain_parser.h"

static size_t trim_left(const char *s, size_t len)
{
    size_t i = 0;

    while (i < len && (s[i] == ' ' || s[i] == '\t')) {
        i++;
    }

    return i;
}

static size_t trim_right(const char *s, size_t start, size_t len)
{
    while (len > start &&
           (s[len - 1] == ' ' ||
            s[len - 1] == '\t' ||
            s[len - 1] == '\r')) {
        len--;
    }

    return len;
}

static bool starts_with_ci(const char *s,
                           size_t len,
                           const char *prefix)
{
    size_t n = strlen(prefix);

    if (n > len) return false;

    for (size_t i = 0; i < n; i++) {
        if (std::toupper((unsigned char)s[i]) !=
            std::toupper((unsigned char)prefix[i])) {
            return false;
        }
    }

    return true;
}

static bool ends_with_ci(const char *s,
                         size_t len,
                         const char *suffix)
{
    size_t n = strlen(suffix);

    if (n > len) return false;

    return starts_with_ci(s + len - n, n, suffix);
}

/*
 * Fountain character cues are conventionally uppercase.
 *
 * This intentionally only treats ASCII letters as uppercase.
 * UTF-8 names are left for a later Unicode-aware enhancement rather
 * than incorrectly classifying arbitrary UTF-8 bytes.
 */
static bool all_caps_ascii(const char *s, size_t len)
{
    bool has_letter = false;

    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];

        if (c >= 0x80) {
            return false;
        }

        if (std::isalpha(c)) {
            has_letter = true;

            if (!std::isupper(c)) {
                return false;
            }
        }
    }

    return has_letter;
}

static bool is_parenthetical(const char *s, size_t len)
{
    return len >= 2 &&
           s[0] == '(' &&
           s[len - 1] == ')';
}

static bool is_note(const char *s, size_t len)
{
    return len >= 4 &&
           s[0] == '[' &&
           s[1] == '[' &&
           s[len - 2] == ']' &&
           s[len - 1] == ']';
}

static bool is_boneyard_marker(const char *s, size_t len)
{
    return
        (len >= 3 &&
         s[0] == '/' &&
         s[1] == '*') ||

        (len >= 2 &&
         s[0] == '*' &&
         s[1] == '/');
}

/*
 * Fountain transitions are normally uppercase and either:
 *
 *     CUT TO:
 *     DISSOLVE TO:
 *     SMASH CUT TO:
 *     FADE OUT
 *     FADE TO BLACK
 *
 * The generic "* TO:" test handles additional conventional
 * transition names.
 */
static bool is_transition(const char *s, size_t len)
{
    if (len == 0 || !all_caps_ascii(s, len)) {
        return false;
    }

    return
        ends_with_ci(s, len, " TO:") ||
        starts_with_ci(s, len, "CUT TO:") ||
        starts_with_ci(s, len, "FADE OUT") ||
        starts_with_ci(s, len, "FADE TO BLACK");
}

/*
 * Character cue heuristic.
 *
 * Explicit @CHARACTER is supported, as is the normal Fountain
 * uppercase-character convention.
 */
static bool looks_like_character(const char *s, size_t len)
{
    if (len == 0 || len > 80) {
        return false;
    }

    if (s[0] == '@') {
        return true;
    }

    if (!all_caps_ascii(s, len)) {
        return false;
    }

    /*
     * A transition is uppercase too, so exclude it here.
     */
    if (is_transition(s, len)) {
        return false;
    }

    return true;
}

/*
 * Fountain scene headings normally begin with:
 *
 *     INT.
 *     EXT.
 *     INT/EXT.
 *     I/E.
 *
 * A leading '.' is also the Fountain forced-scene-heading syntax.
 */
static bool looks_like_scene_heading(const char *s, size_t len)
{
    if (len == 0) {
        return false;
    }

    if (s[0] == '.') {
        return len > 1;
    }

    return
        starts_with_ci(s, len, "INT.") ||
        starts_with_ci(s, len, "EXT.") ||
        starts_with_ci(s, len, "INT/EXT.") ||
        starts_with_ci(s, len, "I/E.") ||
        starts_with_ci(s, len, "EST.") ||
        starts_with_ci(s, len, "INT ") ||
        starts_with_ci(s, len, "EXT ");
}

static void set_info(fountain_line_info_t *info,
                     fountain_line_type_t type,
                     const char *content,
                     size_t content_len,
                     bool forced)
{
    info->type = type;
    info->content = content;
    info->content_len = content_len;
    info->indent_level = 0;
    info->forced = forced;
}

extern "C" void fountain_parser_reset(fountain_parser_state_t *state)
{
    if (state) {
        state->state = FOUNTAIN_STATE_NORMAL;
    }
}

extern "C" bool fountain_is_scene_heading(const char *line, size_t len)
{
    if (!line) {
        return false;
    }

    size_t start = trim_left(line, len);
    size_t end = trim_right(line, start, len);

    if (start >= end) {
        return false;
    }

    return looks_like_scene_heading(line + start, end - start);
}

extern "C" bool fountain_is_page_break(const char *line, size_t len)
{
    if (!line) {
        return false;
    }

    size_t start = trim_left(line, len);
    size_t end = trim_right(line, start, len);
    size_t n = end - start;

    /*
     * Fountain page breaks are conventionally:
     *
     *     ===
     *
     * A single '=' is also accepted here so the parser remains
     * permissive for simple authoring.
     */
    return n > 0 &&
           line[start] == '=' &&
           (n == 1 ||
            (n == 3 &&
             line[start + 1] == '=' &&
             line[start + 2] == '='));
}

extern "C" void fountain_parse_line(const char *line,
                                     size_t len,
                                     fountain_line_info_t *info,
                                     fountain_parser_state_t *state)
{
    if (!info) {
        return;
    }

    memset(info, 0, sizeof(*info));

    info->type = FOUNTAIN_LINE_EMPTY;
    info->content = line;
    info->content_len = len;

    /*
     * Allow NULL state for callers that only need a one-line
     * classification.
     */
    fountain_parser_state_t local_state = {};

    if (!state) {
        state = &local_state;
    }

    size_t start = trim_left(line, len);
    size_t end = trim_right(line, start, len);

    /*
     * Blank lines terminate dialogue context.
     */
    if (start >= end) {
        set_info(info,
                 FOUNTAIN_LINE_EMPTY,
                 line + start,
                 0,
                 false);

        state->state = FOUNTAIN_STATE_NORMAL;
        return;
    }

    const char *s = line + start;
    size_t n = end - start;

    /*
     * Boneyard / block comment.
     */
    if (state->state == FOUNTAIN_STATE_BONEYARD) {
        set_info(info,
                 FOUNTAIN_LINE_BONEYARD,
                 s,
                 n,
                 false);

        if (strstr(s, "*/")) {
            state->state = FOUNTAIN_STATE_NORMAL;
        }

        return;
    }

    if (is_boneyard_marker(s, n) &&
        s[0] == '/' &&
        s[1] == '*') {

        set_info(info,
                 FOUNTAIN_LINE_BONEYARD,
                 s,
                 n,
                 false);

        if (!strstr(s + 2, "*/")) {
            state->state = FOUNTAIN_STATE_BONEYARD;
        }

        return;
    }

    /*
     * [[ Fountain note ]]
     */
    if (is_note(s, n)) {
        set_info(info,
                 FOUNTAIN_LINE_NOTE,
                 s,
                 n,
                 false);

        return;
    }

    /*
     * >Centered text<
     */
    if (s[0] == '>' &&
        n > 1 &&
        s[n - 1] == '<') {

        set_info(info,
                 FOUNTAIN_LINE_CENTERED,
                 s + 1,
                 n - 2,
                 false);

        state->state = FOUNTAIN_STATE_NORMAL;
        return;
    }

    /*
     * Page break.
     */
    if (fountain_is_page_break(s, n)) {
        set_info(info,
                 FOUNTAIN_LINE_PAGE_BREAK,
                 s,
                 n,
                 false);

        state->state = FOUNTAIN_STATE_NORMAL;
        return;
    }

    /*
     * Explicit / conventional scene heading.
     */
    if (looks_like_scene_heading(s, n)) {
        bool forced =
            (s[0] == '.' && n > 1);

        set_info(info,
                 FOUNTAIN_LINE_SCENE_HEADING,
                 s,
                 n,
                 forced);

        state->state = FOUNTAIN_STATE_NORMAL;
        return;
    }

    /*
     * !Action
     *
     * The leading ! forces a line to be treated as action rather
     * than allowing uppercase/other heuristics to classify it.
     */
    if (s[0] == '!' && n > 1) {
        set_info(info,
                 FOUNTAIN_LINE_ACTION,
                 s + 1,
                 n - 1,
                 true);

        state->state = FOUNTAIN_STATE_NORMAL;
        return;
    }

    /*
     * @CHARACTER explicitly forces a character cue.
     */
    if (s[0] == '@' &&
        n > 1 &&
        looks_like_character(s, n)) {

        set_info(info,
                 FOUNTAIN_LINE_CHARACTER,
                 s + 1,
                 n - 1,
                 true);

        state->state = FOUNTAIN_STATE_DIALOGUE;
        return;
    }

    /*
     * Transition lines must be checked before the generic
     * uppercase-character heuristic.
     */
    if (is_transition(s, n)) {
        set_info(info,
                 FOUNTAIN_LINE_TRANSITION,
                 s,
                 n,
                 false);

        state->state = FOUNTAIN_STATE_NORMAL;
        return;
    }

    /*
     * Parentheticals only become screenplay parentheticals when
     * they occur inside dialogue.
     */
    if (is_parenthetical(s, n) &&
        state->state == FOUNTAIN_STATE_DIALOGUE) {

        set_info(info,
                 FOUNTAIN_LINE_PARENTHETICAL,
                 s,
                 n,
                 false);

        return;
    }

    /*
     * Once a character cue has been seen, subsequent non-empty
     * lines belong to that character's dialogue until the next
     * blank line.
     */
    if (state->state == FOUNTAIN_STATE_DIALOGUE) {
        set_info(info,
                 FOUNTAIN_LINE_DIALOGUE,
                 s,
                 n,
                 false);

        return;
    }

    /*
     * In normal Fountain syntax, an uppercase line is a character
     * cue. Authors can force ordinary action with !.
     */
    if (looks_like_character(s, n)) {
        set_info(info,
                 FOUNTAIN_LINE_CHARACTER,
                 s,
                 n,
                 false);

        state->state = FOUNTAIN_STATE_DIALOGUE;
        return;
    }

    /*
     * Everything else is action.
     */
    set_info(info,
             FOUNTAIN_LINE_ACTION,
             s,
             n,
             false);
}

extern "C" const char *fountain_line_type_name(fountain_line_type_t type)
{
    switch (type) {
    case FOUNTAIN_LINE_EMPTY:
        return "empty";

    case FOUNTAIN_LINE_SCENE_HEADING:
        return "scene_heading";

    case FOUNTAIN_LINE_ACTION:
        return "action";

    case FOUNTAIN_LINE_CHARACTER:
        return "character";

    case FOUNTAIN_LINE_DIALOGUE:
        return "dialogue";

    case FOUNTAIN_LINE_PARENTHETICAL:
        return "parenthetical";

    case FOUNTAIN_LINE_TRANSITION:
        return "transition";

    case FOUNTAIN_LINE_CENTERED:
        return "centered";

    case FOUNTAIN_LINE_PAGE_BREAK:
        return "page_break";

    case FOUNTAIN_LINE_NOTE:
        return "note";

    case FOUNTAIN_LINE_BONEYARD:
        return "boneyard";

    default:
        return "unknown";
    }
}
