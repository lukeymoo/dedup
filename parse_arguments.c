#include "parse_arguments.h"

enum
{
  eDIRECTORY
};

void
parse_arguments (int argc, char **argv, char **dir_flag_dst)
{
  // options table
  int c;
  int option_index = 0;
  static struct option long_options[] = {
    { "dir", required_argument, NULL, eDIRECTORY },
    { 0, 0, 0, 0 },
  };

  // parse options
  while (1)
    {
      c = getopt_long (argc, argv, "", long_options, &option_index);
      if (c == -1)
        break;

      switch (c)
        {
        case 0:
          /* retrieve specified directory */
          if (optarg)
            {
              if (*dir_flag_dst)
                {
                  puts ("duplicate --dir argument");
                  show_usage (argv[0]);
                  exit (EXIT_FAILURE);
                }
              *dir_flag_dst = optarg;
            }
          break;
        default:
          show_usage (argv[0]);
          exit (EXIT_FAILURE);
        }
    }
  return;
}
