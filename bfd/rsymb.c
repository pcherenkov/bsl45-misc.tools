/* @(#) read symbols using libbfd from bintools */
/* gcc -W -Wall --pedantic -ggdb -o rsymb -Wl,--no-as-needed -lbfd ./rsymb.c  */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <bfd.h>

static int
read_symbols(const char* fpath, FILE* out)
{
    bfd *h = NULL;
    long storage_needed = -1, nsym = -1, i = 0;
    asymbol **symbol_table = NULL;
    struct bfd_section *sect = NULL;
    unsigned long vma = 0, size = 0, count = 0;
    char **matching = NULL, *s = NULL;

    int rc = -1;

    assert(fpath && out);

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

            (void) fprintf(out, "%ld\t%s vma=%lu size=%lu val=%lu\n", i,
                symbol_table[i]->name, vma, size, symbol_table[i]->value);
            if (vma + symbol_table[i]->value > 0 && symbol_table[i]->value < size)
                ++count;
        }

        (void) fprintf(out, "%s: %lu symbols read\n", fpath, count);

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

