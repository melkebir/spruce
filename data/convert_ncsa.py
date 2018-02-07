#!/usr/bin/python
import sys
import convert

if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.stderr.write("Usage: %s <DCF_input> <CONF>\n" % sys.argv[0])
        sys.exit(1)

    filename = sys.argv[1]
    conf = float(sys.argv[2])
    with open(filename) as f:
        print f.readline(),
        print f.readline(),
        f.readline()
        print "#" + "\t".join(["sample_index", "sample_label", "character_index", "character_label", "vaf_lb", "vaf_mean", "vaf_ub", "mu", "x", "y"])
        for line in f:
            s = line.rstrip("\n").split("\t")
            var = int(s[5])
            ref = int(s[4])
            mode, lower, upper = convert.binomial_hpdr(var, ref + var, conf)
            print "\t".join(map(str, [s[0], s[1], s[2], s[3], lower, mode, upper] + s[6:]))
