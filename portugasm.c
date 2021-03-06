/*
 *  Portugasm
 *  Copyright (c) 2022 Leonardo Zamboni
 *
 *  this program is free software: you can redistribute it and/or modify
 *  it under the terms of the gnu general public license as published by
 *  the free software foundation, either version 3 of the license, or
 *  (at your option) any later version.
 *
 *  this program is distributed in the hope that it will be useful,
 *  but without any warranty; without even the implied warranty of
 *  merchantability or fitness for a particular purpose.  see the
 *  gnu general public license for more details.
 *
 *  you should have received a copy of the gnu general public license
 *  along with this program.  if not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define BUILD_DIR "build"

typedef struct portugasm PtAsm_t;

typedef struct content_tks ContTks_t;

struct content_tks
{
  char *tk;
  ContTks_t *n;
};

/*
 * PTASM TOKEN LIST
 */
struct portugasm
{
  ContTks_t *tks;
  ContTks_t *i;
};

typedef struct tokens Tokens_t;

#define NTKS (sizeof (TK) / sizeof (Tokens_t))

struct tokens
{
  char *asm_tk;
  char *pt_tk;
};

static Tokens_t TK[] =
{
  { "section","secao" },
  { ".data",".dados" },
  { "section .data","secao_dados" },
  { ".bss",".blcinc" },
  { "section .bss","secao_inicial" },
  { ".text",".texto" },
  { "section .text","secao_texto" },
  { "_start","principal" },
  { "_start:","principal:" },
  { ",","<-" },
  /*
   * TYPES
   */
  { "db","1byte" },
  { "dw","2byte" },
  { "dd","4byte" },
  { "dq","8byte" },
  { "dt","10byte" },
  /*
   * INSTRUCTIONS
   */
  { "add","adicionar" },
  { "mov","mover" },
  { "sub","subtrair" },
  { "div","dividir" },
  { "mul","multiplicar" },
  { "ret","retorna" },
  { "syscall","chamadasis" },
  { "call","chamada" },
  { "push","insere" },
  { "pop","retira" },
  { "inc","incremente" },
  { "dec","decremento" },
  { "imult","multiplicarint" },
  { "idiv","dividirint" },
  { "not","nao" },
  { "neg","negar" },
  { "jump","salto" },
  { "cmp","compare" },
};


enum
{
  HELP,
  FELF64,
};

/*
 * AUX FUNCTIONS
 */
size_t
get_command(const char **commands, const char *arg)
{
  if (!(*commands) || strcmp(arg, (*commands)) == 0)
    return 0;
  commands++;
  return 1 + get_command(commands, arg);
}

size_t
get_flag(char *var)
{
  const char *commands[] =
  {
    "-help",
    "-felf64",
    NULL
  };

  return get_command(commands, var);
}

char *
remove_spaces(char *tk)
{
  while ((*tk == ' ') || (*tk == '\t'))
    ++tk;
  return tk;
}
/**/

PtAsm_t *
create ()
{
  PtAsm_t *ptasm = malloc(sizeof(PtAsm_t));
  ptasm->tks = malloc(sizeof(ContTks_t));
  ptasm->tks->n = NULL;
  ptasm->tks->tk = NULL;
  ptasm->i = ptasm->tks;
  return ptasm;
}

void
get_content_tk (PtAsm_t **ptasm, char *tk)
{
  ContTks_t *n = malloc(sizeof(ContTks_t));
  n->n = NULL;
  n->tk = malloc(sizeof(tk));
  strcpy(n->tk, tk);
  (*ptasm)->tks->n = n;
  (*ptasm)->tks = n;
}

void
lex (PtAsm_t **ptasm, FILE *pt)
{
  char bff[255];
  char *tk;

  while (fgets(bff, 255, pt))
    {
      tk = strtok(bff, " ");

      while (tk)
        {

          if (strchr(tk, ' ') || strchr(tk, '\t'))
            {
              tk = remove_spaces(tk);
              get_content_tk (&(*ptasm), tk);
            }
          else if (strchr(tk, '\n'))
            {
              tk[strlen(tk) - 1] = '\0';
              get_content_tk (&(*ptasm), tk);
              get_content_tk (&(*ptasm), "\n");
            }
          else
            {
              get_content_tk (&(*ptasm), tk);
            }


          tk = strtok(NULL, " ");
        }

      if(ferror(pt)) exit (EXIT_FAILURE);
    }
}

void
parser (ContTks_t *in, FILE *src, size_t flag)
{
  if (!in) return;

  if (strcmp(in->tk, "\n") == 0)
    {
      fprintf(src, "%s", in->tk);
    }
  else if (strcmp(in->tk,"\0"))
    {
      Tokens_t tk;
      size_t i;
      _Bool isMain64 = 0;
      _Bool isMain64Colon = 0;

      for (i = 0; i < NTKS; ++i)
        {
          tk = TK[i];
          if (flag == FELF64)
            {
              if (strcmp(in->tk, "principal") == 0)
                {
                  isMain64 = 1;
                  break;
                }
              else if (strcmp(in->tk, "principal:") == 0)
                {
                  isMain64Colon = 1;
                  break;
                }
            }
          if (strcasecmp(in->tk, tk.pt_tk) == 0)
            break;
        }

      if (isMain64 || isMain64Colon)
        {
          fprintf(src, "_start%c ", isMain64Colon ? ':' : ' ');
          isMain64 = 0;
          isMain64Colon = 0;
        }
      else
        {
          if (in->n->tk && strcmp(in->n->tk, "<-") == 0)
            {
              fprintf(src, "%s", i < NTKS ? tk.asm_tk : in->tk);
            }
          else
            {
              fprintf(src, "%s ", i < NTKS ? tk.asm_tk : in->tk);
            }
        }
    }

  parser (in->n, src, flag);
}

int
main (int argc, char **argv)
{
  int flag = -1;
  if (argc > 1)
    flag = get_flag(argv[1]);

  if (flag == HELP)
    {
      printf("-help\n" \
             "Assemble:\n" \
             "\t\t./ptasm [format] <file>\n" \
             "Formats:\n" \
             "\t\t-felf64\t\telf 64 bits file format\n");
      return 0;
    }

  FILE *pt = fopen(argv[argc - 1],"r");
  if (!pt) exit (EXIT_FAILURE);

  char cmd_build_file[
   strlen("mkdir -p ") + strlen(BUILD_DIR)];

  strcpy(cmd_build_file, "mkdir -p ");
  strcat(cmd_build_file, BUILD_DIR);
  system(cmd_build_file);

  char build_file[
   strlen(BUILD_DIR) + strlen(argv[argc - 1]) + 1];

  strcpy(build_file, BUILD_DIR);
  strcat(build_file, "/");
  strcat(build_file, argv[argc - 1]);

  FILE *src = fopen(build_file,"w");
  if (!src) exit (EXIT_FAILURE);

  PtAsm_t *ptasm = create();

  lex(&ptasm, pt);

  parser (ptasm->i->n, src, flag);

  fclose (src);
  fclose (pt);

  return 0;
}
