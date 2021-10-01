#define OPENSSL_API_COMPAT 30100
#define OPENSSL_NO_DEPRECATED
#include <openssl/md5.h>

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wordexp.h>

#include "parse_arguments.h"
#include "show_usage.h"

struct binary_data_t
{
  char *data;
  unsigned size;
};

struct file_entry_t
{
  char *path;
  char *hash;
  long size;
};

struct files_container_t
{
  struct file_entry_t *data;
  unsigned count;
  unsigned capacity;
};

static char *directory = NULL;
struct files_container_t files;

struct binary_data_t
get_file_binary (const char *path)
{
  assert (path);

  struct stat s;
  if (stat (path, &s) == -1)
    {
      perror ("error opening file");
      exit (EXIT_FAILURE);
    }

  char *buf = calloc (s.st_size, sizeof (char));

  FILE *f = NULL;
  if ((f = fopen (path, "rb")) == NULL)
    {
      perror ("failed to open file");
      exit (EXIT_FAILURE);
    }
  fread (buf, s.st_size, 1, f);
  fclose (f);

  struct binary_data_t data = {
    .data = buf,
    .size = s.st_size,
  };

  return data;
}

static void
init_files_container (void)
{
  assert (!files.data);
  files.data = calloc (1000, sizeof (struct file_entry_t));
  files.capacity = 1000;
  files.count = 0;
  return;
}

static void
append_files_container (struct file_entry_t *e)
{
  assert (e);
  assert (e->path);

  if (files.count < files.capacity)
    {
      memcpy (&files.data[files.count++], e, sizeof (struct file_entry_t));
      return;
    }

  /* double the allocation space & zero the new spaces */

  files.capacity *= 2;
  // clang-format off
  files.data = realloc (files.data,
                        sizeof (struct file_entry_t) * files.capacity);
  // clang-format on
  unsigned c = files.count;
  for (; c < files.capacity; c++)
    memset (&files.data[c], 0, sizeof (struct file_entry_t));

  /* insert the new allocation */

  memcpy (&files.data[files.count++], e, sizeof (struct file_entry_t));
  printf ("stored size: %ld\n", files.data[files.count - 1].size);
  return;
}

static void
cleanup_files_container (void)
{
  if (!files.data)
    return;

  unsigned i = 0;
  for (; i < files.count; i++)
    {
      free (files.data[i].path);
      free (files.data[i].hash);
    }

  free (files.data);

  memset (&files, 0, sizeof (files));
  return;
}

int
main (int argc, char *argv[])
{
  static char buffer[PATH_MAX + 32];
  memset (buffer, 0, sizeof (buffer));

  /* get target directory */

  parse_arguments (argc, argv, &directory);

  if (!directory)
    {
      show_usage (argv[0]);
      exit (EXIT_FAILURE);
    }

  if (optind < argc)
    {
      puts ("invalid arguments specified");
      show_usage (argv[0]);
      exit (EXIT_FAILURE);
    }

  /* expand directory string */

  wordexp_t expand;
  wordexp (directory, &expand, 0);
  size_t i = 0;
  if (i < expand.we_wordc)
    directory = strdup (expand.we_wordv[0]);
  else
    directory = strdup (directory);
  wordfree (&expand);

  /* get directory contents & store absolute paths in array */

  DIR *dir = NULL;
  struct dirent *entry;

  if ((dir = opendir (directory)) == NULL)
    {
      // clang-format off
      printf ("failed to open specified directory %s: %s\n",
              directory,
              strerror (errno));
      exit (EXIT_FAILURE);
      // clang-format on
    }

  chdir (directory);

  init_files_container ();

  unsigned filecount = 0;

  while ((entry = readdir (dir)) != NULL)
    {
      // clang-format off
      if (strcmp (entry->d_name, ".") == 0 ||
          strcmp (entry->d_name, "..") == 0)
        {
          // clang-format on
          continue;
        }

      if (entry->d_type != DT_REG || entry->d_name[0] == '.')
        continue;

      memset (buffer, 0, sizeof (buffer));

      realpath (entry->d_name, buffer); // get absolute path

      // get file size
      struct stat s;
      if (stat (buffer, &s) == -1)
        {
          perror ("error getting file attributes");
          exit (EXIT_FAILURE);
        }

      struct file_entry_t e = {
        .path = strdup (buffer),
        .size = s.st_size,
        .hash = NULL,
      };

      append_files_container (&e);
      filecount++;
    }

  closedir (dir);

  for (unsigned i = 0; i < files.count; i++)
    {
      char hashbuf[128];
      unsigned char digest[MD5_DIGEST_LENGTH];
      memset (digest, 0, sizeof (digest));
      memset (hashbuf, 0, sizeof (hashbuf));

      struct binary_data_t data = get_file_binary (files.data[i].path);

      MD5_CTX ctx;
      MD5_Init (&ctx);
      MD5_Update (&ctx, data.data, data.size);
      MD5_Final (digest, &ctx);

      free (data.data);

      int d = 0;
      char *b = hashbuf;
      for (; d < 16; d++) // ps..im not this clever; stackoverflow ftw :3
        b += sprintf (b, "%02x", digest[d]);

      files.data[i].hash = strdup (hashbuf);
      printf ("hashing file %d of %d\n", i, filecount);
    }

  /* write hashes and filenames to file */

  puts ("writing hashes to file...");

  FILE *fh = NULL;
  if ((fh = fopen ("moo-dup-hashes.txt", "w")) == NULL)
    {
      perror ("error opening hashes file");
      exit (EXIT_FAILURE);
    }

  for (unsigned i = 0; i < files.count; i++)
    {
      // clang-format off
      fwrite (files.data[i].path,
              sizeof (char),
              strlen (files.data[i].path),
              fh);
      // clang-format on
      fputc (',', fh);
      fwrite (files.data[i].hash, sizeof (char), strlen (files.data[i].hash),
              fh);
      fputc ('\n', fh);
    }

  fclose (fh);

  puts ("...done");

  free (directory);
  cleanup_files_container ();

  return 0;
}
