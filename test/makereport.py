#!/usr/bin/python

import os
import sys
import time
import subprocess
import argparse
from collections import OrderedDict

def find_time_unit(t):
    if t < 0.000001:
        return 'ns'
    if t < 0.001:
        return '\xc2\xb5s'
    if t < 0.1:
        return 'ms'
    return 's'

def convert_time(t, unit):
    if unit == 'ns':
        return t / 1000000000.0
    if unit == '\xc2\xb5s':
        return t / 1000000.0
    if unit == 'ms':
        return t / 1000.0
    return t

def parse_log(report, reportpath):
    counts = []
    with open(reportpath, 'rb') as f:
        n = -1
        iterations = -1
        for line in f:
            if line.startswith('# n '):
                tokens = line.split()
                n = int(tokens[2])
                iterations = int(tokens[4])
                if not n in counts:
                    counts.append(n)
                continue
            if line.startswith('#'):
                continue

            tokens = line.split()
            name = tokens[0] # container name
            testname = tokens[1]
            if not testname in report['timings']:
                report['memory'][testname]      = OrderedDict()
                report['allocations'][testname] = OrderedDict()
                report['timings'][testname]     = OrderedDict()
            
            if not name in report['timings'][testname]:
                report['memory'][testname][name]      = list()
                report['allocations'][testname][name] = list()
                report['timings'][testname][name]     = list()
                
            if 'used' == tokens[2]:
                memory      = int(tokens[3])
                allocations = int(tokens[6])
                report['memory'][testname][name].append(memory)
                report['allocations'][testname][name].append(allocations)
                
            else: # timings
                index       = tokens.index("min:") # avg, median, min, max
                timing      = float(tokens[index+1])
                unit        = tokens[index+2]
                timing      = convert_time(timing, unit)
                report['timings'][testname][name].append(timing)
    return counts


def collect_table_data(counts, report, tabledata):

    for category, tests in report.iteritems():
        for testname, results in tests.iteritems():
            if testname in ['title', 'scale', 'unit']:
                continue
            
            if not category in tabledata:
                tabledata[category] = OrderedDict()
            if not testname in tabledata[category]:
                tabledata[category][testname] = OrderedDict()
            
            if not 'counts' in tabledata[category][testname]:
                tabledata[category][testname]['counts'] = list()
            tabledata[category][testname]['counts'].extend(counts)
            
            for name, values in results.iteritems():
                if not name in tabledata[category][testname]:
                    tabledata[category][testname][name] = list()
                if name in ['title', 'scale', 'unit']:
                    tabledata[category][testname][name] = values
                    continue
                tabledata[category][testname][name].extend(values)
            

def make_table_report(data):
    usediff = False

    for category, tests in data.iteritems():
    
        totaldiff = 0.0
    
        for testname, results in tests.iteritems():
            if testname in ['title', 'scale', 'formatter', 'unit']:
                continue
            
            columns = list()
            for name, values in results.iteritems():
                if len(values) < len(results['counts']):
                    values.extend( (len(results['counts']) - len(values)) * [0.0])
                columns.append( [name]+values )
            
            formatter = tests['formatter']
            scale = tests['scale']
            title = tests['title']
            
            matrix = zip(*columns)
            
            rows = [list(matrix[0])]
            for row in matrix[1:]:
                rows.append( [str(row[0])] + map(formatter, map(lambda x: scale * x, row[1:]) ) )
            
            lengths = [0] * len(rows[0])
            for row in rows:
                for ic, v in enumerate(row):
                    lengths[ic] = max(lengths[ic], len(v))
            
            # header
            headers = []
            headersunderline = []
            for ic, v in enumerate(rows[0]):
                length = lengths[ic]
                headers.append( ' ' + v.ljust(length) + ' ' )
                if ic == 0:
                    headersunderline.append( '-' * (length + 1) + ':' )
                else:
                    headersunderline.append( '-' * (length + 2) )
                    
            print "## " + title + " " + testname
            print ""
            print '|' + '|'.join(headers) + '|'
            print '|' + '|'.join(headersunderline) + '|'

            for row in rows[1:]:
                values = []
                for ic, v in enumerate(row):
                    length = lengths[ic]
                    value = v.ljust(length)
                    values.append( ' ' + value + ' ')
                
                print '|' + '|'.join(values) + '|',
                if not usediff:
                    print ""
                
                diff = 0.0
                if usediff:
                    tokens = values[-1].split()
                    diff = float(tokens[0]) - float(values[-2].split()[0])
                    print diff, tokens[1]
        
            if usediff:            
                totaldiff += diff
            
            print ""
            print ""
        
        if usediff:
            print "Total diff:", totaldiff


def make_timings_report(input_path):
    report = OrderedDict()
    report['timings'] = OrderedDict()
    report['memory'] = OrderedDict()
    report['allocations'] = OrderedDict()
    
    report['timings']['title'] = 'Timings (microseconds)'
    report['timings']['scale'] = 1000000.0
    report['timings']['unit']  = 'us'
    report['memory']['title'] = 'Memory (kb)'
    report['memory']['scale'] = 1 / (1024.0 * 1024.0)
    report['memory']['unit']  = 'mb'
    report['allocations']['title'] = 'Num Allocations'
    report['allocations']['scale'] = 1
    report['allocations']['unit']  = ''
        
    counts = parse_log(report, input_path)

    tabledata = OrderedDict()
    tabledata['timings'] = OrderedDict()
    tabledata['memory'] = OrderedDict()
    tabledata['allocations'] = OrderedDict()
    
    collect_table_data(counts, report, tabledata)

    #del tabledata['memory']
    #del tabledata['allocations']

    tabledata['timings']['title'] = 'Timings'
    tabledata['timings']['scale'] = 1000.0
    tabledata['timings']['formatter'] = lambda x: '%.4f ms' % x
    if 'memory' in tabledata:
        tabledata['memory']['title'] = 'Memory'
        tabledata['memory']['scale'] = 1 / 1024.0
        tabledata['memory']['formatter']  = lambda x: '%d kb' % x
    if 'allocations' in tabledata:
        tabledata['allocations']['title'] = 'Num Allocations'
        tabledata['allocations']['scale'] = 1
        tabledata['allocations']['formatter'] = lambda x: str(x)

    # Render to output to table format
    make_table_report(tabledata)


def make_file_size_report(path, regex):
    print path, regex

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Process some integers.')
    parser.add_argument('-m', '--mode', default='timings', choices=['timings', 'filesize'], help='Enables file size report')
    parser.add_argument('--regex', nargs='+', help='Matches to be made')
    parser.add_argument('-i', '--input', help='The input file/directory')
    args = parser.parse_args()

    if not args.input:
        print("Need a log file from the test run")
        sys.exit(1)

    timestart = time.time()

    if args.mode == 'filesize':
        make_file_size_report(args.input, args.regex)
    else:
        make_timings_report(args.input)

    timeend = time.time()
    print "# Report made in %f seconds" % (timeend - timestart)
