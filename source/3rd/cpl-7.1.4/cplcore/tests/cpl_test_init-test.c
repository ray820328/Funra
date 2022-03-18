/*
 * This file is part of the ESO Common Pipeline Library
 * Copyright (C) 2001-2017 European Southern Observatory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <cpl_test.h>

#ifdef HAVE_UNISTD_H
/* Used for fork(), pipe(), read() and write() */
#include <unistd.h>
#endif

#include <sys/resource.h>

/*-----------------------------------------------------------------------------
                                  Defines
 -----------------------------------------------------------------------------*/

#define NUMBER_OF_FILENAMES 26

/*-----------------------------------------------------------------------------
                              Private functions
 -----------------------------------------------------------------------------*/

#ifdef HAVE_UNISTD_H

/* The pipe on which the child writes the log file name to the parent. */
int pipefd[2] = {-1, -1};


/* The signal handler for writing the log file name in the child process. We do
   the writing in a signal handler at the abort stage since the
   cpl_test_init_macro() routine that we are testing might abort if it can
   not create a file with the filename it generated during the test. */
static void signal_handler(int sig)
{
    if (sig == SIGABRT) {
        ssize_t total_written = 0;
        
        /* We are assuming that cpl_msg_get_log_name() is reentrant and safe for
           a signal handler. Since this only happens once at abort it probably
           does not matter much anyway, even if it technically is not safe. */
        const char* logname = cpl_msg_get_log_name();
        
        /* Write the message taking into account partial writes.
           Note: on the child side we only ever touch pipefd in the signal
           handler so no chance of a race condition. Plus these routines are all
           fine to use in signal handlers according to POSIX. */
        do {
            ssize_t bytes_written = write(pipefd[1], logname+total_written,
                                          strlen(logname)+1-total_written);
            if (bytes_written < 0) break;
            total_written += bytes_written;
        } while (total_written != (ssize_t)(strlen(logname)+1));

        /* Do not bother testing fsync() or close() success in the child since
           we are not interested in reporting such errors. We have anyway closed
           all the standard input and output files. If there was a failure then
           the parent process will anyway receive an incorrect string and print
           an error. */
        fsync(pipefd[1]);
        close(pipefd[0]);
        close(pipefd[1]);

        /* Stop the logging to make sure the log file is closed to prevent
           unnecessary valgrind warnings. */
        cpl_msg_stop_log();
    }
}

#endif /* HAVE_UNISTD_H */

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
#ifdef HAVE_UNISTD_H

    int pipe_create_result = 0;
    int pipe_close_result = 0;
    pid_t child_pid = 0;
    ssize_t bytes_read = 0;
    int current_file = 0;
    
    /* The filenames to check. */
    const char* filename[NUMBER_OF_FILENAMES] = {
        "",
        ".",
        "..",
        "../",
        "file",
        "/file",
        "./file",
        "./file.c",
        "./file.cxx",
        "./file.name",
        "./file.name.c",
        "/dir/file",
        "/dir/file.c",
        "/dir/file.cxx",
        "/dir.name/file",
        "..\\",
        "\\file",
        ".\\file",
        ".\\file.c",
        ".\\file.cxx",
        ".\\file.name",
        ".\\file.name.c",
        "\\dir\\file",
        "\\dir\\file.c",
        "\\dir\\file.cxx",
        "\\dir.name\\file"
    };
    
    /* List of expected filenames. */
    const char* expected_filename[NUMBER_OF_FILENAMES] = {
        ".log",
        ".log",
        ".log",
        ".log",
        "file.log",
        "file.log",
        "file.log",
        "file.log",
        "file.log",
        "file.log",
        "file.name.log",
        "file.log",
        "file.log",
        "file.log",
        "file.log",
        ".log",
        "file.log",
        "file.log",
        "file.log",
        "file.log",
        "file.log",
        "file.name.log",
        "file.log",
        "file.log",
        "file.log",
        "file.log"
    };

    /* Buffer containing the generated filename returned by the child. */
    char generated_filename[1024];
    
    /* We run the test on cpl_test_init_macro(). However, this is a bit tricky
       since we can only make one call to cpl_test_init_macro() per process.
       In addition, that routine might abort if a failure is triggered, which
       will be the case if the routine is buggy. Thats exactly what we want to
       test and catch. The solution is to fork off children processes that will
       make the test call to cpl_test_init_macro(). However this means that the
       parent cannot make a call to cpl_test_init() until all the children have
       been forked, since they will have a copy of the state at the point of the
       fork. We just have to perform all the tests silently in the parent and on
       the first sign of error we stop, initialise the CPL test library and
       print the diagnostics.
       We use a pipe to deliver the file name actually created by the
       cpl_test_init_macro() routine in the child to the parent process. */
    
    for (current_file = 0; current_file < NUMBER_OF_FILENAMES; ++current_file) {

        pipe_create_result = pipe(pipefd);
        if (pipe_create_result != 0) break;
        child_pid = fork();
        if (child_pid == -1) break;
        
        if (child_pid == 0) {
            
            /* Child process code: */
            
            struct sigaction siginfo;
            struct rlimit rlim;
            int result = 0;
            const char * fname = filename[current_file];

            /* The child will always be calling the abort command so make sure
               we do not create core dumps */
            result = getrlimit(RLIMIT_CORE, &rlim);
            rlim.rlim_cur = 0;
            result |= setrlimit(RLIMIT_CORE, &rlim);
            if (result != 0) {
                fname = "FAILED";
            }
            
            /* Catch the abort signal that will be generated by the child either
               in cpl_test_init_macro because the log file could not be created
               or from the explicit call afterwards. */
            memset(&siginfo, 0x0, sizeof(siginfo));
            siginfo.sa_handler = &signal_handler;
            sigemptyset(&siginfo.sa_mask);
            sigaction(SIGABRT, &siginfo, NULL);
            
            /* Close all std(in)(out)(err) so the child does not write there. */
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
            
            cpl_test_init_macro(fname, PACKAGE_BUGREPORT, CPL_MSG_OFF);
            
            /* Children always abort so that we can then handle the write to the
               output pipe in one place (the signal handler). */
            abort();
            
        } else {
            
            /* Parent process code: */
            
            size_t total_read = 0;
            
            /* Make the filename a null terminated string by filling the buffer
               with zeros. */
            memset(&generated_filename, 0x0, sizeof(generated_filename));
            
            /* The parent has to close the write end of the pipe before trying
               to read from it. If we do not do this we could deadlock, because
               only when all file descriptors for the write end of the pipe get
               closed by the child and parent, will the read fail. Otherwise it
               will block. The read and write end of the pipe in the child is
               closed in one go in its signal_handler.
               Note that we actually test if this close() was successful because
               we must make sure the write end is indeed closed to avoid any
               deadlock. */
            pipe_close_result = close(pipefd[1]);
            if (pipe_close_result != 0) break;
            
            /* Read the filename string from the child. We keep going if we get
               interrupted or until a NULL terminator is read. */
            do {
                bytes_read = read(pipefd[0], generated_filename + total_read,
                                  sizeof(generated_filename)-1-total_read);
                if (bytes_read > 0) {
                    total_read += bytes_read;
                    if (generated_filename[total_read-1] == '\0') break;
                }
            } while (bytes_read == -1 && errno == EINTR);
            
            /* Can now close the pipe. Do not bother checking the status of the
               file closure, since any error here means that generated_filename
               contains a value that was not expected and an error will be
               generated later anyway. */
            close(pipefd[0]);
            
            /* Try remove the generated log file. We do not check the return
               status since the file might not actually have been generated,
               so we will fail in remove() in such a case. */
            remove(generated_filename);
            
            if (bytes_read < 0) break;  /* Had a read error? */
            
            /* Check that we got the correct response from the child. */
            if (strcmp(generated_filename,
                       expected_filename[current_file]) != 0) {
                break;
            }
        }
    }
    
    /* We can finally initialise the CPL subsystem in the parent since no more
       children will be created, check the status and print test diagnostics. */
    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);
    
    cpl_test_assert(pipe_create_result == 0);
    cpl_test_assert(pipe_close_result == 0);
    cpl_test_assert(child_pid != -1);
    cpl_test_assert(bytes_read >= 0);
    if (current_file < NUMBER_OF_FILENAMES) {
        /* Check that the child was able to set itself up correcly, e.g. set
           the RLIMIT_CORE value. Then generate the message for unexpected
           filenames. */
        cpl_test_assert(strstr(generated_filename, "FAILED")
                        != generated_filename);
        cpl_test_eq_string(generated_filename, expected_filename[current_file]);
    }
    cpl_test_eq(current_file, NUMBER_OF_FILENAMES);

    return cpl_test_end(0);

#else /* HAVE_UNISTD_H */
    
    /* In the case that we dont have unistd.h we just write a warning. */
    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);
    cpl_msg_warning(cpl_func,
                   "No support for unistd.h, so this test cannot be completed");
    cpl_test_end(0);
    return 77;  /* 77 is used by automake to indicate a skipped test. */

#endif /* HAVE_UNISTD_H */
}
