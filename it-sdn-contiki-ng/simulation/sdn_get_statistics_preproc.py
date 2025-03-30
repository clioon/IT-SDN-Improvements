import re, sys

def preproc_file(filename):
    print "Parsing", filename

    # expression = re.compile(r"""
    #     (?P<TIME>[\d:\.,]+) # First field is time, we do not match any pattern
    #     (?:\s.*ID:|:) # ID field is different in Cooja GUI and command line (ID: or just a :)
    #     (?P<ID>\d+) # the actual id is a sequence of numbers
    #     (?:\s|:).* # finishing id pattern (colon or space) and matching everything else until getting what we want
    #     .* # address field
    #     (?:(?:=(?P<RTX>FG|RX|TX(?:A|B|)))|(?P<ENER>E:\s\d+\smJ)) # RX or TX
    #     """, re.X) #re.X: verbose so we can comment along

    expression = re.compile(r"""
        =(?:FG|RX|TX[AB]?)|E:\s\d+\smJ # RX or TX
        """, re.X) #re.X: verbose so we can comment along

    try:
        f = file(filename)
    except:
        print "error on f = file(filename)"
        return

    try:
        f_out = file(filename + 'preproc', 'w')
    except:
        print "error on f = file(filename)"
        return

    for l in f:
        # print l.strip()
        s = expression.search(l)
        if s != None:
            # print s.groups()
            # time, printid, srcaddr, rxtx, messageType, seqNum, dest = s.groups()
            f_out.write(l)
    f_out.close()



if __name__ == "__main__":

    if len(sys.argv) == 2:
        preproc_file(sys.argv[1])
        exit(0)

    nodes_v = (9, 16, 25)
    nodes_v = (9, 16)
    nodes_v = (25, )
    topologies = ('GRID-FULL', 'GRID-RND', 'GRID-CTA')
    nd_possibilities = ('CL', 'NV')
    datarates=(1,)
    MIN_ITER=1
    MIN_ITER=5
    MAX_ITER=10
    # MAX_ITER=5
    results_dir = "results/"

    for nnodes in nodes_v:
        for topo in topologies:
            for nd in nd_possibilities:
                for datarate in datarates:
                    scenario = (nnodes, topo, nd, datarate)
                    for i in range(MIN_ITER, MAX_ITER+1):
                        filename = "cooja_sbrt2017_n" + repr(nnodes) + "_top" + topo + "_nd" + nd + "_l" + repr(datarate) + "_i" + repr(i) + '.txt'
                        preproc_file(results_dir + filename)
