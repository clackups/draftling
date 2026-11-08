// End-to-end host test of git_sync.cpp against a real bare repository.
#include "git_sync.cpp"
#include <cassert>
#include <sys/stat.h>

extern const char *g_http_root;
static std::string T;   /* test root */
static int fails = 0;
static std::string last_msg;
static git_sync_state_t last_state;
static void cb(git_sync_state_t st, const char *m) { last_state = st; if (m) last_msg = m; }

static void sh(const std::string &c)
{
    std::string full = "cd '" + T + "' && " + c + " >/dev/null 2>&1";
    if (system(full.c_str()) != 0) { fprintf(stderr, "FAILED: %s\n", c.c_str()); exit(2); }
}
static std::string out(const std::string &c)
{
    std::string full = "cd '" + T + "' && " + c + " 2>/dev/null";
    FILE *p = popen(full.c_str(), "r"); std::string s; char b[512]; size_t r;
    while ((r = fread(b, 1, sizeof b, p)) > 0) s.append(b, r);
    pclose(p); return s;
}
static void put(const std::string &rel, const std::string &data)
{ FILE *f = fopen((T + "/" + rel).c_str(), "wb"); fwrite(data.data(), 1, data.size(), f); fclose(f); }
static std::string get(const std::string &rel)
{ char *d; size_t n; if (sd_card_read_file((T + "/" + rel).c_str(), &d, &n) != ESP_OK) return "<missing>";
  std::string s(d, n); free(d); return s; }
static bool exists(const std::string &rel) { return sd_card_file_exists((T + "/" + rel).c_str()); }
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "  CHECK failed %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static bool sync_now(const char *proj, git_sync_direction_t how = GIT_SYNC_BOTH)
{
    last_state = GIT_SYNC_IDLE; last_msg.clear();
    s_state = GIT_SYNC_IDLE;
    std::string dir = T + "/sd/" + proj;
    /* git_sync_start() hands the work to the worker task; run it here. */
    if (!load_config(&s_cfg, dir.c_str())) return false;
    s_state = GIT_SYNC_IN_PROGRESS;
    run_sync(how);
    fprintf(stderr, "  sync %s: %s\n", proj, last_msg.c_str());
    return last_state == GIT_SYNC_SUCCESS;
}
static void fresh(const char *name)
{
    fprintf(stderr, "== %s\n", name);
    sh("rm -rf remote.git work sd && mkdir -p sd && git init -q --bare -b main remote.git");
}
static void seed_remote(const std::string &cmds)
{
    sh("git clone -q remote.git work && cd work && git config user.email t@t && git config user.name t && " + cmds +
       " && git add -A && git commit -qm seed && git push -q origin HEAD:main");
}
static void project(const char *p)
{
    sh(std::string("mkdir -p sd/") + p);
    put(std::string("sd/") + p + "/git.cfg", "repo_url=http://fake/remote.git\nbranch=main\n");
}
static std::string remote_file(const std::string &path) { return out("git --git-dir=remote.git show main:" + path); }
static std::string remote_ls() { return out("git --git-dir=remote.git ls-tree -r --name-only main"); }

int main(int argc, char **argv)
{
    char buf[512]; T = std::string(getcwd(buf, sizeof buf)) + "/run"; mkdir(T.c_str(), 0777);
    g_http_root = T.c_str();
    git_sync_set_callback(cb);

    /* 1. The first sync keeps local drafts and the server's other files. */
    fresh("first sync with local drafts");
    seed_remote("echo server-a > a.md && echo server-b > b.md && printf 'PNG' > img.png && "
                "mkdir sub && echo deep > sub/c.md && echo '*.tmp' > .gitignore");
    project("P");
    put("sd/P/a.md", "local-a\n");
    put("sd/P/mine.md", "only here\n");
    CHECK(sync_now("P"));
    CHECK(get("sd/P/mine.md") == "only here\n");
    CHECK(get("sd/P/b.md") == "server-b\n");
    CHECK(get("sd/P/a.md").find("local-a") != std::string::npos);
    CHECK(get("sd/P/a.md").find("server-a") != std::string::npos);
    CHECK(!exists("sd/P/img.png"));
    std::string ls = remote_ls();
    CHECK(ls.find("mine.md") != std::string::npos);
    CHECK(ls.find("img.png") != std::string::npos);
    CHECK(ls.find("sub/c.md") != std::string::npos);
    CHECK(ls.find(".gitignore") != std::string::npos);
    CHECK(remote_file("mine.md") == "only here\n");

    /* 2. A later edit keeps the server's non-document files. */
    put("sd/P/b.md", "edited\n");
    CHECK(sync_now("P"));
    ls = remote_ls();
    CHECK(remote_file("b.md") == "edited\n");
    CHECK(ls.find("img.png") != std::string::npos && ls.find("sub/c.md") != std::string::npos);

    /* 3. Server-side change to a non-document and a document. */
    sh("cd work && git pull -q origin main && echo new-b > b.md && printf 'PNG2' > img.png && "
       "git commit -qam s2 && git push -q origin HEAD:main");
    put("sd/P/mine.md", "only here, edited\n");
    CHECK(sync_now("P"));
    CHECK(get("sd/P/b.md") == "new-b\n");
    CHECK(remote_file("img.png") == "PNG2");
    CHECK(remote_file("mine.md") == "only here, edited\n");

    /* 4. A git.cfg committed to the repository does not reach the card. */
    sh("cd work && git pull -q origin main && echo 'repo_url=http://evil/x' > git.cfg && "
       "git add git.cfg && git commit -qm cfg && git push -q origin HEAD:main");
    CHECK(sync_now("P"));
    CHECK(get("sd/P/git.cfg").find("fake") != std::string::npos);

    /* 5. First sync of an empty project against a populated server. */
    fresh("first sync, empty project");
    seed_remote("echo one > one.md && echo two > two.txt");
    project("Q");
    CHECK(sync_now("Q"));
    CHECK(get("sd/Q/one.md") == "one\n" && get("sd/Q/two.txt") == "two\n");
    put("sd/Q/one.md", "one!\n");
    CHECK(sync_now("Q"));
    CHECK(remote_file("one.md") == "one!\n");

    /* 6. First sync into an empty repository. */
    fresh("first sync, empty repository");
    project("R");
    put("sd/R/x.md", "x\n");
    CHECK(sync_now("R"));
    CHECK(remote_file("x.md") == "x\n");

    /* 7. Many files: none is lost. */
    fresh("200 files");
    project("M");
    for (int i = 0; i < 200; i++) put("sd/M/f" + std::to_string(i) + ".md", std::to_string(i) + "\n");
    CHECK(sync_now("M"));
    int n = 0; ls = remote_ls();
    for (size_t p = 0; (p = ls.find(".md", p)) != std::string::npos; p++) n++;
    CHECK(n == 200);

    /* 8. More than 32 refs on the server. */
    fresh("many refs");
    seed_remote("echo s > s.md && for i in $(seq 1 60); do git tag t$i 2>/dev/null || true; done");
    sh("cd work && for i in $(seq 1 60); do git branch aaa$i; done && git push -q --all origin && git push -q --tags origin");
    project("B");
    put("sd/B/s.md", "s2\n");
    CHECK(sync_now("B"));
    CHECK(remote_file("s.md").find("s2") != std::string::npos);

    /* 9. Branch names that would leave .git/refs are refused. */
    fresh("bad branch");
    project("X");
    put("sd/X/git.cfg", "repo_url=http://fake/remote.git\nbranch=../../../evil\n");
    put("sd/X/x.md", "x\n");
    CHECK(git_sync_start((T + "/sd/X").c_str(), GIT_SYNC_BOTH) == ESP_ERR_INVALID_STATE);
    CHECK(last_msg == "Invalid branch name in git.cfg");
    CHECK(git_branch_name_ok("main") && git_branch_name_ok("feature/x-1"));
    CHECK(!git_branch_name_ok("a..b") && !git_branch_name_ok("/a") && !git_branch_name_ok("a.lock") &&
          !git_branch_name_ok("a b") && !git_branch_name_ok("a/.b") && !git_branch_name_ok(""));

    /* 10. Rebase of many unpushed local commits (pull-only syncs). */
    fresh("70 local commits");
    seed_remote("echo base > s.md");
    project("L");
    CHECK(sync_now("L"));
    for (int i = 0; i < 70; i++) {
        put("sd/L/l.md", "local " + std::to_string(i) + "\n");
        if (!sync_now("L", GIT_SYNC_PULL)) { CHECK(false); break; }
    }
    sh("cd work && git pull -q origin main && echo remote > r.md && git add r.md && "
       "git commit -qm r && git push -q origin HEAD:main");
    CHECK(sync_now("L"));
    CHECK(remote_file("l.md") == "local 69\n" && remote_file("r.md") == "remote\n");

    /* 11. Documents the card cannot hold are skipped, not fatal, and
     *     stay as they are on the server. */
    fresh("unwritable documents");
    seed_remote("echo ok > ok.md && echo q > 'what?.md' && ln -s ok.md link.md");
    project("U");
    CHECK(sync_now("U"));
    CHECK(last_msg.find("2 not copied") != std::string::npos);
    CHECK(get("sd/U/ok.md") == "ok\n" && !exists("sd/U/what?.md") && !exists("sd/U/link.md"));
    put("sd/U/ok.md", "ok2\n");
    CHECK(sync_now("U"));
    CHECK(remote_file("ok.md") == "ok2\n" && remote_file("what?.md") == "q\n");
    CHECK(out("git --git-dir=remote.git ls-tree main link.md").find("120000") == 0);

    /* 12. An edited document keeps its mode in the repository. */
    fresh("file modes");
    seed_remote("echo x > exec.md && chmod +x exec.md");
    project("E");
    CHECK(sync_now("E"));
    put("sd/E/exec.md", "x2\n");
    CHECK(sync_now("E"));
    CHECK(remote_file("exec.md") == "x2\n");
    CHECK(out("git --git-dir=remote.git ls-tree main exec.md").find("100755") == 0);

    /* 13. A mode change on the server survives a rebase of a local
     *     edit of the same file. */
    fresh("mode merge");
    seed_remote("echo m > m.md && echo o > o.md");
    project("G");
    CHECK(sync_now("G"));
    sh("cd work && git pull -q origin main && chmod +x m.md && git commit -qam mode && "
       "git push -q origin HEAD:main");
    put("sd/G/m.md", "m2\n");
    CHECK(sync_now("G"));
    CHECK(remote_file("m.md") == "m2\n");
    CHECK(out("git --git-dir=remote.git ls-tree main m.md").find("100755") == 0);

    /* 14. git_commit_list_linear(): a base off the first-parent path
     *     is refused instead of listing everything back to the root. */
    {
        fprintf(stderr, "== linear range\n");
        sh("rm -rf lin && mkdir lin");
        CHECK(git_repo_open((T + "/lin").c_str()) == ESP_OK);
        git_oid tree, a, b, c, both[2];
        CHECK(git_odb_write(GIT_OBJ_TREE, "", 0, &tree) == ESP_OK);
        const char *id = "t <t@t> 1735689600 +0000";
        CHECK(git_commit_create(&tree, NULL, 0, id, "a", &a) == ESP_OK);
        CHECK(git_commit_create(&tree, NULL, 0, id, "c", &c) == ESP_OK);
        both[0] = a; both[1] = c;                      /* merge: c is 2nd parent */
        CHECK(git_commit_create(&tree, both, 2, id, "b", &b) == ESP_OK);
        git_oid *list = NULL; int n = -1;
        CHECK(git_commit_list_linear(&a, &b, 0, &list, &n) == ESP_OK && n == 1);
        free(list); list = NULL;
        CHECK(git_commit_list_linear(&c, &b, 0, &list, &n) == ESP_ERR_NOT_FOUND);
        free(list); list = NULL;
        CHECK(git_commit_list_linear(NULL, &b, 0, &list, &n) == ESP_OK && n == 2);
        free(list);
        git_repo_close();
    }

    fprintf(stderr, fails ? "\n%d check(s) FAILED\n" : "\nall checks passed\n", fails);
    return fails ? 1 : 0;
}
