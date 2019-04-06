#!/bin/sh

# This is an example of how simple it would be to make an external metric.
# The first and only argument is the filepath to the graph; the output is a
# single line to stdout, consisting of the score. The metric name is the
# filename without extension (so this one would show up as "Filesize" in score
# files).
#
# This file is not marked as executable, so it will not be picked up by
# graph_analyser as is. This is because the file size is not a very useful
# metric in practice. Simply `chmod +x' the file to have it show up in the
# list of metrics when running `graph_analyser -l' and usable as a metric.

du -b $1 | cut -f1
