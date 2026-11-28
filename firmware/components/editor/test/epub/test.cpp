/* Host test of epub.cpp. See run.sh. */
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "epub.h"

static std::string chapter(epub_book_t *b, int i, size_t cap = 1 << 20)
{
    std::string out(cap, '\0');
    const char *why = NULL;
    long n = epub_chapter_markdown(b, i, &out[0], cap, &why);
    if (n < 0) {
        fprintf(stderr, "chapter %d: %s\n", i, why);
        abort();
    }
    out.resize((size_t)n);
    return out;
}

static void expect(const std::string &md, const char *needle)
{
    if (md.find(needle) == std::string::npos) {
        fprintf(stderr, "missing: [%s]\n--- in:\n%s---\n", needle, md.c_str());
        abort();
    }
}

static void reject(const std::string &md, const char *needle)
{
    if (md.find(needle) != std::string::npos) {
        fprintf(stderr, "unexpected: [%s]\n--- in:\n%s---\n", needle, md.c_str());
        abort();
    }
}

static std::string md(const char *x)
{
    std::string out(4096, '\0');
    out.resize(epub_xhtml_to_markdown(x, strlen(x), &out[0], out.size()));
    return out;
}

int main(void)
{
    const char *why = NULL;
    epub_book_t *b = epub_open("test.epub", &why);
    assert(b);
    assert(!strcmp(epub_title(b), "The & Test Book"));
    assert(epub_chapter_count(b) == 3);
    assert(!strcmp(epub_chapter_title(b, 0), "One"));
    assert(!strcmp(epub_chapter_title(b, 1), "Two parts"));   /* nested span, #fragment */
    assert(epub_chapter_title(b, 2) == NULL);                  /* only in the landmarks nav */

    std::string c1 = chapter(b, 0);
    printf("--- chapter 1 ---\n%s--- end ---\n", c1.c_str());
    reject(c1, "Skip me");
    reject(c1, "color");
    reject(c1, "no</p>");
    reject(c1, "hidden");
    expect(c1, "# Chapter One\n\n");
    expect(c1, "It was a *dark* and **stormy** night\xE2\x80\xA6 \xE2\x80\x9CQuoted\xE2\x80\x9D & done.\n");
    expect(c1, "Line one\nLine two\n");
    expect(c1, "*Whole italic sentence.* After.");
    expect(c1, "> A quote.\n");
    expect(c1, "- First\n- Second\n- Third\n");
    expect(c1, "  1. Nested a\n  2. Nested b\n");
    expect(c1, "\n---\n");
    expect(c1, "\\# not a heading, 2 * 3 = 6, snake_case, a\\*b, 1. not a list");
    expect(c1, "1\\. Starts with a number");
    expect(c1, "```\n  code   kept\n  as is\n```");
    expect(c1, "`x*y`");
    expect(c1, "[image: A map]");
    expect(c1, "softhyphen");
    expect(c1, "[image]");   /* the svg */

    std::string c2 = chapter(b, 1);
    assert(c2 == "## Two\n\nSecond chapter.\n");

    /* A chapter larger than the buffer is cut short with a note. */
    std::string c3 = chapter(b, 2, 4096);
    assert(c3.size() <= 4096);
    expect(c3, "[The rest of this chapter does not fit in memory.]");
    assert(chapter(b, 2).size() > 30000);

    assert(epub_chapter_markdown(b, 3, NULL, 0, &why) == -1);
    epub_close(b);

    b = epub_open("epub2.epub", &why);
    assert(b);
    assert(!strcmp(epub_title(b), "Old Book"));
    assert(epub_chapter_count(b) == 1);
    assert(!strcmp(epub_chapter_title(b, 0), "Alpha"));
    assert(chapter(b, 0) == "Hello\n");
    epub_close(b);

    assert(!epub_open("notzip.epub", &why) && why);
    assert(!epub_open("missing.epub", &why) && why);

    /* Fragments without a body, emphasis across a break, entities. */
    assert(md("<p>a<b>b</b>c</p>") == "a**b**c\n");
    assert(md("<p><i>x<br/>y</i></p>") == "*x*\n*y*\n");
    assert(md("<p>&unknown; &#x263A; &#9731;</p>") == "&unknown; \xE2\x98\xBA \xE2\x98\x83\n");
    assert(md("<p><b><i>both</i></b></p>") == "***both***\n");
    assert(md("<p><em> </em>x</p>") == "x\n");
    assert(md("<table><tr><td>a</td><td>b</td></tr><tr><td>c</td></tr></table>") ==
           "a | b\nc\n");
    printf("epub host test: OK\n");
    return 0;
}
