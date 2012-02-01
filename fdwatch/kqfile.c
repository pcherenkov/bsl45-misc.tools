#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>

/*
 * example of kqueue EVFILT_VNODE by Peter Werner <peterw@ifost.org.au>
 */

extern char *__progname;

#define NFILES 5

struct kqfile {
	int fd;		/* file descriptor */
	char *path;	/* full path eg /var/log/messages */
	char *name;	/* last component of path eg messages */
} files[NFILES];

int kq;

/* set kevents for a file */
void
register_kevents(struct kqfile *f)
{
	struct kevent ke;

	EV_SET(&ke, f->fd, EVFILT_VNODE, EV_ADD,
	    NOTE_DELETE | NOTE_RENAME, 0, f);

	if (kevent(kq, &ke, 1, NULL, 0, NULL) == -1)
		err(1, "kevent %s", f->path);

	EV_SET(&ke, f->fd, EVFILT_READ, EV_ADD, 0, 0, f);

	if (kevent(kq, &ke, 1, NULL, 0, NULL) == -1)
		err(1, "kevent %s", f->name);
}

/*
 * get the last component of a path 
 */
char *
get_filename(char *nm)
{
	char *tmp;

	tmp = strrchr(nm, '/');
	if (tmp != NULL) {
		tmp++; /* skip the '/' */
		return(tmp);
	}
	return(nm);
}

/* 
 * open a file for reading and seek to the end. return the filedescriptor 
 */
int
open_file(char *path)
{
	int fd;

	fd = open(path, O_RDONLY);
	if (fd == -1)
		return(-1);

	if (lseek(fd, 0, SEEK_END) == -1)
		return(-1);

	return(fd);
}

/*
 * reopen a file after it has been renamed or 
 * deleted ala what logrotate would do, sleep 
 * now and then to give logrotate a change to 
 * create the new file.
 */
int
kqfreopen(struct kqfile *f)
{
	int i;

	close(f->fd);	

	for (i = 0; i < 3; i++) { 

		usleep(100000);

		f->fd = open(f->path, O_RDONLY);
		if (f->fd != -1)
			break;
	}
	
	if (f->fd == -1)
		return(-1);

	register_kevents(f);

	return(f->fd);
}

/* close up a file */
void
kqfremove(struct kqfile *f)
{
	close(f->fd);
	memset(f, 0x00, sizeof(struct kqfile));
}

/*
 * read data one byte at a time untill a newline is reached (or buffer full)
 */
int
rdline(int fd, char *buf, size_t len)
{
	int i, j;

	for (i = 0; i < len; ) {

		j = read(fd, &buf[i], 1);
		if (j == -1 || j == 0) 
			return(j);

		if (buf[i++] == '\n')
			break;
	}

	return(i);
}

/*
 * read the new lines from a file
 */
int
rwfile(struct kqfile *f, char *buf, size_t len, int nbytes) 
{
	int i, total = 0;

	printf("%10.10s: ", f->name);

	do  {
		
		i = rdline(f->fd, buf, len);
		if (i == -1)
			return(-1);
		
		if (write(STDOUT_FILENO, buf, i) != i)
			warn("write stdout");
		else
			total += i;

		/* 
		 * if this is true rdline has read one whole line 
		 * from the file, so print the filename again 
		 */
		if (i < len && total < nbytes)
			printf("%10.10s: ", f->name);

		memset(buf, 0x00, len);

	} while (i > 0 && total < nbytes);

	return(total);
}

/* loop receiving kevents */
void
kq_loop(void)
{
	struct kqfile *f; 
	struct kevent ke;
	int i;
	char buf[BUFSIZ];

	while (1) {

		memset(&ke, 0x00, sizeof(ke));
		memset(buf, 0x00, sizeof(buf));
		
		i = kevent(kq, NULL, 0, &ke, 1, NULL);
		if (i == -1)
			err(1, "kevent");

		f = (struct kqfile *)ke.udata;

		/* data to read */
		if (ke.filter == EVFILT_READ) {

			if (ke.data < 0) {
				printf("%s has shrunk\n", f->path);
				lseek(f->fd, 0, SEEK_END);
				continue;
			}

			i = rwfile(f, buf, sizeof(buf), ke.data); 
			if (i == -1)
				warn("read %s", f->path);

		} else if (ke.filter == EVFILT_VNODE && 
		    (ke.fflags & NOTE_DELETE || ke.fflags & NOTE_RENAME)) {
			if (kqfreopen(f) == -1) {
				warn("%s went away and didnt come back", 
				    f->path);
				kqfremove(f);
			}
		} 
	}
}

int
main(int argc, char **argv)
{	
	int i, fd;
	char *fname;

	kq = kqueue();
	if (kq == -1)
		err(1, "kq!");

	for (i = 0; i < argc - 1 && i < NFILES; i++) {

		fd = open_file(argv[i + 1]);
		if (fd == -1)  { 
			warn("open_file %s", argv[i + 1]);
			continue;
		}

		fname = get_filename(argv[i + 1]);

		files[i].fd = fd;
		files[i].path = argv[i + 1];
		files[i].name = fname;

		register_kevents(&files[i]);
	}

	if (i == NFILES) 
		warnx("max no of files is %d", NFILES);
	else if (i == 0)
		errx(1, "%s <file1> [<file2> .. <file%d>]", __progname, NFILES);
		
	setvbuf(stdout, NULL, _IONBF, 0);

	kq_loop();

	return(0);
}
