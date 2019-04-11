# argsolveranalyser
AAF and solver benchmarking and analysis toolkit.

# Quick start
To build from source: ensure the necessary dependencies (CMake, Boost, a recent version of GCC/Clang) are installed; in the root `CMakeLists.txt`, configure `INSTALL\_PREF` as desired; `cd build` and run `make install`.

Then place some graphs into store/graphs (or use the -g/-d options to specify graphs), run `graph\_analyser` to generate metric scores and/or `benchmarker` with proper options (see `benchmarker -h`) to generate solver performance data, and then either peruse the store or generate an report on the data using `report\_generator -t <type> [other options]` (see `report\_generator -h` for more information).

## Installation
### Prerequisites
* Cmake
* Boost.Program\_options
* A C++17 compatible compiler
### Configuration
A number of variables can be configured in the root CMakeLists.txt:
* INSTALL\_PREF: This is the installation prefix which will be used when running `make install`. By default, it will be `/usr/local/` - or it can be set to any directory, if a portable, self-contained installation that does not require root access is desired.
* STORE\_PATH: This will be the location of the store (see below). It might be a good idea to ensure the location set here has enough free disk space to store large solutions. If configuring for a self-contained installation, set this to a relative path.
* CONF\_PREFIX: This will be the directory where default configuration files will be located. As above, for a portable installation this should be relative, whereas for a systemwide installation you might wish to set this to a subfolder of `~/.config/` or similar, to taste.
The individual default configuration filenames for the different components are also customisable.
### Installation
Once the directories are configured as desired, change to the `/build` directory and run `make install`.

## Usage
The basic idea is as follows: graphs are ran through `graph_analyser`, which generates various metric scores. `benchmarker` is then run, generating data on solver performance. At each stage, all data is saved inside the store directory. At the end, the data can be analysed, correlating solver performance to metric scores.

For each tool, the `--help/-h` option can be used for a detailed description of every option.

### Specifying graphs
By default, `/store/graphs/` is (recursively) searched for files. Alternatively, the `--graphs/-g` option can be used to specify a list of individual files, or the `--graph-dirs/-d` option can be used to specfify a list of directories to (non-recursively) search. In either case, the store is disabled; to instead use both any files from the store *and* files/directories specified manually, add the `--use-store` flag.

### Benchmark run IDs
When running a benchmark, the `--run-id/-i` option can be specified. This simply creates a subdirectory of the same name inside `store/benchmarks`, allowing separation of result sets over multiple benchmark runs. By default, the `default` ID is used. If benchmarker is called with the special ID value `TIMESTAMP`, the current timestamp is used as a run ID: this is useful e.g. for automated runs as part of a script, and/or periodic runs used to track solver performance during development.

The `--recover/-R` option will cause any existing solutions with the same ID to not be re-ran. Otherwise, if an ID is used which already exists, previous results will be overwritten.

IDs are per-solver: multiple different solvers can be benchmarked with the same run-id without the results conflicting or being overwritten. A "solver" for this purpose is identified by the last component (i.e. the filename) of the `--solver-executable` argument; this does mean that if different versions of the same solver are used, and the executables have the same filenames, they will be treated as the same solver and overwrite each other's results.

Finally, `--clobber/-C` can be used to completely wipe any previous results with the specified run ID, rather than merely overwriting them as necessary. This has the effect of making sure that no old results remain, even if a previous run with this ID used graphs that the current run isn't using: without specifing `--clobber`, those results would remain, since they would not be overwritten.

### Formats
Graph files are only supported in TGF (Trivial Graph Format). A TGF file looks like this:
```
a1
a2
a3
#
a1 a2
a1 a3
```
Here, every line up until the `#` identifies a unique argument. After the `#`, every line specified two arguments, separated by whitespace, which define an attack (a directed edge in the argumentation graph). The only restriction is that argument IDs cannot contain newlines or whitespace, and an argument cannot be named literally `#`, as all of these would make parsing ambiguous - no other restrictions are placed on IDs.

### Solver interface
The `benchmarker` follows the probo interface for solvers, which is the same one used in the ICCMA competition. This means that solvers are called as follows:
```
solver-binary -f <INPUT FILE PATH> -fo tgf -p <PROBLEM TO SOLVE> [-a <ADDITIONAL ARGUMENT ID>]
```
Where the arguments should be self-explanatory. For decision problems (credulous and skeptical acceptance), the additional argument is specified as part of the problem definition, in one of the following ways, to avoid having to specify a mapping between every input graph file, every problem, and an argument:
* Randomly, with persistance: a colon `:` is appended to the problem specification (for instance, using again ICCMA problem codes, one would specify `DC-PR:` as the problem for credulous decision under preferred semantics). For every input graph, a random argument is selected, and then cached for future runs: the results are therefore reproducible and deterministic.
* Randomly, without persistance: as above, except after the colon, a hyphen `-` is used. Using the same example as above, the problem would then be `DC-PR:-`. This generates a new random argument even if a cached value is present (and the cache is then updated to this new value).
* By name: a (non-numeric) argument ID can also be specified after the colon, e.g. `DC-PR:a1`. For every graph file, if such an argument exists in the file (for instance, in the TGF example presented above), that argument will be selected; otherwise a random one is used.
* By position: if a number *n* is specified after the colon (e.g. `DC-PR:5`), the *n*th argument as present in the TGF file is used. If there are less than *n* arguments in the file, the last one is used.

#### Solver output
Solver output is also assumed to follow the ICCMA interface. Specifically:
* For decision problems: solvers must output `YES` or `NO`, literally, and nothing else (other than newlines).
* For enumeration problems: each extension must be output as follows: `[a1,a2,a3]` - in other words, the arguments in the extension comma-separated, and enclosed in square brackets. Each extension, in turn, must be comma-separated and the entire list enclosed in square brackets. So, for instance, a full solution might be `[[a1],[a1,a2],[]]`. If no extensions exist, that would be represented by `[]` - as opposed to a single extensions consisting of the empty set, which would be `[[]]`.

#### Wrapper scripts
If a solver does not support this exact format, a wrapper shell script can be constructed relatively trivially to interpret the arguments and transform the output. The wrapper script would call the actual solver internally, and the script would be specified as the "solver binary" in the `benchmarker` call.

### Metrics
When analysing graphs, metrics can be whitelisted or blacklisted, with the blacklist taking precedence. Existing results will only be recalculated if either the `--force-recalulate/-f` or `--clobber/-C` options are specified: in the former case, any old results will be overwritten, but due to the possibility of the set of metrics calculated being different, some old results may remain. As with benchmark results, `--clobber/-C` can thus be used to wipe the results file before running anything else, ensuring all the values will be fresh.

#### External metrics
The list of metrics is meant to be extensible: it is simple to add a new class to `graph_analyser/metrics/`, following the existing template and the `metric.h` interface, and then `#include` the new file into `main.cpp` and then push it into the vector of metrics following the existing examples. However, this requires most of `graph_analyser` to be recompiled every time a change is made, which may be less than convenient.

As an alternative, any executable file in the directory `store/external-metrics` will also be loaded as a metric. The metric name in this case will be the filename of the executable, without the extension. The executable should take exactly one argument, the path to the graph file, and print out to stdout the score, which should be parseable as a `double`. It should exit with a status of 0: any other exit status will result in an error, and the value returned (if any will be rejected).

This allows new metrics to be easily created without recompiling any other code; it also gives a lot of flexibility in how metrics are developed - the executable could be anything from a Python script to a wrapper calling an external tool located elsewhere.

### Graph hashing
Graphs are hashed with SpookyHash (http://burtleburtle.net/bob/hash/spooky.html) to ensure no duplication. `graph_mapper` can be used to quickly check what a graph's hash is, or what graph file a hash belongs to. The file `store/graphhashmap` stores key-value pairs between graph filepath and hash, as a cache, and can also be easily explored or `grep`ed manually.

## Store
The store is where all the data is saved; here is an overview of its structure.
### `graph-scores`
Each file here corresponds to a graph. The format of the files should be self-explanatory: each line gives the value of a metric.
### `bench-solutions`
Here the solutions for every problem are stored, in subdirectories corresponding to each graph. Each problem has its own file, named after the problem, containing the output of the reference solver used verbatim
### `benchmarks`
The output of benchmark runs. Each subfolder here corresponds to a solver. Inside a solver's folder, there is a subfolder for every run ID, inside of which the actual run information is stored. Each benchmark run has a subfolder for every graph, inside which each problem has either 1 or 2 corresponding files named after it: a `<PROBLEM>.stat` file and a `<PROBLEM>.output` file. The latter contains the solver output verbatim, and may not be kept depending on the options passed to `benchmarker` (by default, only outputs for incorrect solutions are kept; you can also specify the max size of solutions to keep). The .stat file contains the performance data of the solver.

# License
This program is released under the GNU General Public License, version 3 or (at your option) any later version. For more information, see LICENSE.txt or https://www.gnu.org/licenses/.
