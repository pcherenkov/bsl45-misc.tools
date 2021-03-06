/* @(#) read symbols using libbfd from bintools */
/* gcc -W -Wall --pedantic -ggdb -o rsymb -Wl,--no-as-needed -lbfd ./rsymb.c  */
/* gcc -W -Wall --pedantic -ggdb -I/opt/local/include -L/opt/local/lib -o rsymb -lbfd ./rsymb.c */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <bfd.h>

static int s_val;
int g_val;

struct frame {
    struct frame *next;
    void *ret;
};


#if defined(USE_RBP)
#define GET_CURRENT_FRAME(f)                \
    do {                                    \
        register void *__f asm("rbp");      \
        f = __f;                            \
    } while(0)
#elif defined(__GNUC__)
#define GET_CURRENT_FRAME(f)                \
    do {                                    \
        f = __builtin_frame_address(0);     \
    } while(0)
#endif


static int
read_symbols(const char* fpath, FILE* out)
{
    bfd *h = NULL;
    long storage_needed = -1, nsym = -1, i = 0;
    asymbol **symbol_table = NULL;
    struct bfd_section *sect = NULL;
    unsigned long vma = 0, size = 0, count = 0, mask = BSF_FUNCTION;
    char **matching = NULL, *s = NULL;
    const char *status = NULL;
#ifdef __GNUC__
    void *frame_addr = NULL;
#endif

    int rc = -1;
    unsigned long isfunc = 0;

    assert(fpath && out);
    s_val = g_val = 1;

#ifdef __GNUC__
    GET_CURRENT_FRAME(frame_addr);
    (void) fprintf(out, "%s frame = %p", __func__, frame_addr);
    if (frame_addr) {
        fprintf(out, " return_addr=%p", ((struct frame*)frame_addr)->ret);
    }
    (void) fputc('\n', out);
#endif

    do {
        bfd_init();

        h = bfd_openr(fpath, NULL);
        if (NULL == h) {
            bfd_perror("bfd_openr");
            break;
        }

        if (TRUE == bfd_check_format(h, bfd_archive)) {
            (void) fprintf(out, "%s is an archive\n", fpath);
            break;
        }

        if (FALSE == bfd_check_format_matches(h, bfd_object, &matching)) {
            bfd_perror("bfd_check_format_matching");
            if (!matching)
                break;

            for(s = *matching; s; ++s)
                (void) fprintf(out, "%s ", s);

            (void) fputc('\n', out);
            free(matching);
        }

        if (bfd_target_elf_flavour == h->xvec->flavour)
            (void) fputs("ELF file format\n", out);
        else
            (void) fprintf(out, "file format: %ld\n",
                        (long)h->xvec->flavour);

        storage_needed = bfd_get_symtab_upper_bound(h);
        if (storage_needed <= 0) {
            if (storage_needed < 0)
                bfd_perror("bfd_get_symtab_upper_bound");
            break;
        }

        symbol_table = malloc(storage_needed);
        if (NULL == symbol_table) {
            perror("out of memory - malloc");
            break;
        }

        nsym = bfd_canonicalize_symtab(h, symbol_table);
        if (nsym < 0) {
            bfd_perror("bfd_canonicalize_symtab");
            break;
        }

        for (i = 0, count = 0; i < nsym; ++i) {
            sect = bfd_get_section(symbol_table[i]);
            if (NULL == sect) {
                bfd_perror("bfd_get_section");
                break;
            }

            vma = bfd_get_section_vma(h, sect);
            size = bfd_get_section_size(sect);
            isfunc = (bfd_target_elf_flavour == h->xvec->flavour) ?
                        (unsigned long)symbol_table[i]->flags & mask : 1;

            if (isfunc && ((vma + symbol_table[i]->value) > 0) &&
                (symbol_table[i]->value < size)) {
                    ++count;
                    status = "GOOD";
            }
            else
                status = "BAD";

            (void) fprintf(out, "%ld\t%s\t%lu\t%s\t[0x%08lx + %lu] {vma=0x%08lx val=0x%lx size=0x%lx flags=0x%08lx}\n",
                i+1, status, vma + symbol_table[i]->value, symbol_table[i]->name,
                vma + symbol_table[i]->value, size,
                vma, symbol_table[i]->value, size,
                (unsigned long)symbol_table[i]->flags);
        }

        (void) fprintf(out, "%s: %lu/%lu symbols read/good\n", fpath, nsym, count);

        rc = 0;
    } while(0);


    if (TRUE != bfd_close(h))
        (void) fprintf(out, "error closing BFD\n");

    if (symbol_table)
        free(symbol_table);

    return rc;
}


int main(int argc, char* const argv[])
{
    if (argc < 2) {
        (void) fprintf(stderr, "Usage: %s filepath\n", argv[0]);
        return 1;
    }

    return read_symbols(argv[1], stdout);
}

/* __EOF__ */

