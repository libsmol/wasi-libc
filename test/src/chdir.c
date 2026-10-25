#include "test.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define TEST(c)                                                                \
  do {                                                                         \
    errno = 0;                                                                 \
    if (!(c))                                                                  \
      t_error("%s failed (errno = %d)\n", #c, errno);                          \
  } while (0)

static int exists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0;
}

// Checks that the working directory is `below` under `start`.
static void expect_cwd(const char *start, const char *below) {
  char expected[PATH_MAX];
  char actual[PATH_MAX];
  int root = strcmp(start, "/") == 0;
  snprintf(expected, sizeof(expected), "%s%s", root ? "" : start, below);
  if (expected[0] == 0)
    strcpy(expected, "/");

  TEST(getcwd(actual, sizeof(actual)) != NULL);
  if (strcmp(actual, expected) != 0)
    t_error("getcwd: expected \"%s\", got \"%s\"\n", expected, actual);
}

static void spray_heap(void) {
#define N 100
  void *ptrs[N];
  for (int i = 0; i < N; i++) {
    size_t size = (rand() % 256) + 16;
    ptrs[i] = malloc(size);
    memset(ptrs[i], 0x42, size);
  }
  for (int i = 0; i < N; i++) {
    free(ptrs[i]);
  }
}

int main(void) {
  spray_heap();

  char start[PATH_MAX];
  TEST(getcwd(start, sizeof(start)) != NULL);

  int fd;
  TEST(mkdir("a", 0755) == 0);
  TEST(mkdir("a/b", 0755) == 0);
  TEST(mkdir("a/b/c", 0755) == 0);

  TEST((fd = open("a1.txt", O_CREAT | O_WRONLY, 0644)) >= 0);
  TEST(close(fd) == 0);
  TEST((fd = open("a/a2.txt", O_CREAT | O_WRONLY, 0644)) >= 0);
  TEST(close(fd) == 0);
  TEST((fd = open("a/b/a3.txt", O_CREAT | O_WRONLY, 0644)) >= 0);
  TEST(close(fd) == 0);
  TEST((fd = open("a/b/c/a4.txt", O_CREAT | O_WRONLY, 0644)) >= 0);
  TEST(close(fd) == 0);

  TEST(exists("a1.txt"));
  TEST(exists("a/a2.txt"));
  TEST(exists("a/b/a3.txt"));
  TEST(exists("a/b/c/a4.txt"));

  TEST(chdir("a") == 0);
  TEST(exists("../a1.txt"));
  TEST(exists("a2.txt"));
  TEST(exists("b/a3.txt"));
  TEST(exists("b/c/a4.txt"));

  TEST(chdir("b") == 0);
  TEST(exists("../../a1.txt"));
  TEST(exists("../a2.txt"));
  TEST(exists("a3.txt"));
  TEST(exists("c/a4.txt"));

  TEST(chdir("c") == 0);
  TEST(exists("../../../a1.txt"));
  TEST(exists("../../a2.txt"));
  TEST(exists("../a3.txt"));
  TEST(exists("a4.txt"));

  TEST(chdir("..") == 0);
  TEST(exists("../../a1.txt"));
  TEST(exists("../a2.txt"));
  TEST(exists("a3.txt"));
  TEST(exists("c/a4.txt"));

  TEST(chdir("..") == 0);
  TEST(exists("../a1.txt"));
  TEST(exists("a2.txt"));
  TEST(exists("b/a3.txt"));
  TEST(exists("b/c/a4.txt"));

  TEST(chdir("..") == 0);
  TEST(exists("a1.txt"));
  TEST(exists("a/a2.txt"));
  TEST(exists("a/b/a3.txt"));
  TEST(exists("a/b/c/a4.txt"));

  TEST(chdir("a/b") == 0);
  TEST(exists("../../a1.txt"));
  TEST(exists("../a2.txt"));
  TEST(exists("a3.txt"));
  TEST(exists("c/a4.txt"));
  expect_cwd(start, "/a/b");

  // The working directory never keeps "." or ".." components
  TEST(chdir("..") == 0);
  expect_cwd(start, "/a");
  TEST(exists("a2.txt"));

  TEST(chdir("./b/../b/.//c/") == 0);
  expect_cwd(start, "/a/b/c");
  TEST(exists("a4.txt"));

  TEST(chdir("../../..") == 0);
  expect_cwd(start, "");
  TEST(exists("a1.txt"));

  TEST(chdir("a/../a/b/c") == 0);
  TEST(chdir("../../../a") == 0);
  expect_cwd(start, "/a");
  TEST(exists("a2.txt"));

  return t_status;
}
