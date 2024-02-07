#include <sstream>
#include <iomanip>
#include <iostream>
#include <string>

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <limits.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>

using std::ostringstream;
using std::fixed;
using std::cerr; using std::clog; using std::cout; using std::endl;
using std::string;

string message_prefix;

void usage (char *argv0) {
  cerr << "Usage :" << argv0 << " [-a] [-cs] [-io] [-pf] [-rss] [-t] command [command options and file redirections]" << endl
       << "   -a: all of the following options:" << endl
       << "  -cs: context switches" << endl
       << "  -io: io" << endl
       << "  -pf: page faults" << endl
       << " -rss: resident set size" << endl
       << "   -t: real, user, and system times" << endl
       << "At least one of the above options must be specified."
       << endl;
}

int main (int argc, char *argv[])
{
  struct timeval tstart, tstop;

// Set up the error message prefix string

   pid_t pid = getpid ();
#if defined HOST_NAME_MAX
   char hostname[HOST_NAME_MAX];
   if (gethostname (hostname, HOST_NAME_MAX) == -1) {
     cerr << argv[0] << "(PID " << pid << "): "
          << "gethostname: " << strerror (errno) << "."
          << endl;
     exit (EXIT_FAILURE);
  }
#elif defined _POSIX_HOST_NAME_MAX
   char hostname[_POSIX_HOST_NAME_MAX];
   if (gethostname (hostname, _POSIX_HOST_NAME_MAX) == -1) {
     cerr << argv[0] << "(PID " << pid << "): "
          << "gethostname: " << strerror (errno) << "."
          << endl;
     exit (EXIT_FAILURE);
  }
#endif

  ostringstream emps;
  emps << argv[0] << " (PID " << pid << " on host " << hostname << "): ";
  message_prefix = emps.str ();

// Issue a message if this invocatoin does nothing

  if (argc == 1) {
    usage (argv[0]);
    exit (EXIT_FAILURE);
  }

// process the option list

  int i_exec;

  bool cs  = false;
  bool io  = false;
  bool pf  = false;
  bool rss = false;
  bool t   = false;

  int n_options = 0;
  for (int i = 1; i < argc; ++i) {
    string argvi = string (argv[i]);
    if (argvi[0] != '-') {
      i_exec = i;
      break;
    }
         if (argvi == "-a") {
      cs  = true; io  = true;
      pf  = true; rss = true;
      t   = true;
      n_options++;
    }
    else if (argvi == "-cs" ) {
      cs  = true; n_options++;
    }
    else if (argvi == "-io" ) {
      io  = true; n_options++;
    }
    else if (argvi == "-pf" ) {
      pf  = true; n_options++;
    }
    else if (argvi == "-rss") {
      rss = true; n_options++;
    }
    else if (argvi == "-t"  ) {
      t   = true; n_options++;
    }
    else {
      usage (argv[0]);
      exit (EXIT_FAILURE);
    }
  }
  if (n_options == 0) {
    usage (argv[0]);
    exit (EXIT_FAILURE);
  }

  if (t) {
    if (gettimeofday (&tstart, NULL) == -1) {
      cerr << message_prefix
           << "gettimeofday (tstart): " << strerror (errno) << "."
           << endl;
      exit (EXIT_FAILURE);
    }
  }

  pid_t child_pid = fork ();

  if (child_pid == -1) {

// Call failed

    cerr << message_prefix
         << "fork: " << strerror (errno) << "."
         << endl;
    exit (EXIT_FAILURE);

  }
  else if (child_pid == 0) {

// Code executed by child

    if (execvp (argv[i_exec], &argv[i_exec]) == -1) {
      cerr << message_prefix
           << "execvp (" << argv[i_exec] << "): " << strerror (errno) << "."
           << endl;
      exit (EXIT_FAILURE);
    }

  }
  else {

// Code executed by parent

    int status;
    pid_t wait_pid = wait (&status);

    if (t) {
      if (gettimeofday (&tstop, NULL) == -1) {
        cerr << message_prefix
             << "gettimeofday (tstop): " << strerror (errno) << "."
             << endl;
        exit (EXIT_FAILURE);
      }
    }

    if (wait_pid == -1) {
      if (errno == ECHILD)
        clog << message_prefix
             << "wait: " << strerror (errno) << "."
             << endl;
      else {
        cerr << message_prefix
             << "wait: " << strerror (errno) << "."
             << endl;
        exit (EXIT_FAILURE);
      }
    }

    ostringstream output;
    output.str ("");

    output << argv[0] << ": " << argv[i_exec] << " (PID " << child_pid << " on host " << hostname << "): ";

// Some of these branches will never be taken in the
// current implementation; they are for a rewrite
// using waitpid

         if (WIFEXITED (status))
           output << " exited with status " << WEXITSTATUS (status) << ".";
    else if (WIFSIGNALED (status))
           output << " terminated by signal " << WTERMSIG (status) << " (" << strsignal (WTERMSIG (status)) << ").";
    else if (WIFSTOPPED (status))
           output << " stopped by signal " << WSTOPSIG (status) << " (" << strsignal (WSTOPSIG (status)) << ").";
    else if (WIFCONTINUED (status))
           output << " continued.";
    else
           output << ": unknown status.";

    clog << output.str () << endl;


    struct rusage ru;
    if (getrusage (RUSAGE_CHILDREN, &ru) == -1) {
      cerr << message_prefix
           << "getrusage: " << strerror (errno) << "."
           << endl;
      exit (EXIT_FAILURE);
    }

    if (cs ) {
      output.str ("");
      output << "Context switches (voluntary, involuntary):   " << ru.ru_nvcsw << ", " << ru.ru_nivcsw;
      clog << output.str () << endl;
    }
    if (io ) {
      output.str ("");
      output << "File system block I/O ops (input, output):   " << ru.ru_inblock << ", " << ru.ru_oublock;
      clog << output.str () << endl;
    }
    if (pf ) {
      output.str ("");
      output << "Page faults (hard (I/O req), soft (no I/O)): " << ru.ru_majflt << ", " << ru.ru_minflt;
      clog << output.str () << endl;
    }
    if (rss) {
      output.str ("");
#if defined(__LINUX__) || defined(__linux__) || defined(__FreeBSD__)
      // Linux reports ru_maxrss in units of kilobytes (KB).
      output << "Maximum resident set size (in Mbytes):       " << ((double) ru.ru_maxrss) / 1024.;
#elif defined(__APPLE__)
      // macOS reports ru_maxrss in units of bytes.
      output << "Maximum resident set size (in Mbytes):       " << ((double) ru.ru_maxrss) / (1024.*1024.);
#endif
      clog << output.str () << endl;
    }
    if (t) {
      double real =   (((double) tstop.tv_sec ) + 1.0e-6 * ((double) tstop.tv_usec))
                    - (((double) tstart.tv_sec) + 1.0e-6 * ((double) tstart.tv_usec));
      double user = ((double) ru.ru_utime.tv_sec) + 1.0e-6 * ((double) ru.ru_utime.tv_usec);
      double sys  = ((double) ru.ru_stime.tv_sec) + 1.0e-6 * ((double) ru.ru_stime.tv_usec);
      output.str ("");
      output << "Time (real/user/sys in seconds):             " << fixed << real << "/" << user << "/" << sys;
      clog << output.str () << endl;
    }

  }

  return EXIT_SUCCESS;
}
