#! /usr/bin/python

import os
import sys
import subprocess
from collections import OrderedDict
import plotly.plotly as py
from plotly.graph_objs import Data, Figure, XAxis, YAxis, Bar, Layout, Font, Legend


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


def run_test(report, *args):
    cmd = ['./test']+map(str, args)
    print "cmd:", ' '.join(cmd)
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    p.wait()
    d = p.stdout.read()
    
    lines = d.replace('\r', '\n').split('\n')
    
    for line in lines[1:]:
        tokens = line.split()
        if not tokens:
            continue
        
        name = tokens[0]
        if not name in report['timings']:
            report['memory'][name]      = list()
            report['allocations'][name] = list()
            report['timings'][name]     = list()
            
        if 'used' == tokens[1]:
            memory      = int(tokens[2])
            allocations = int(tokens[5])
            report['memory'][name].append(memory)
            report['allocations'][name].append(allocations)
            
        else: # timings
            timing      = float(tokens[4])
            unit        = tokens[5]
            timing      = convert_time(timing, unit)
            report['timings'][name].append(timing)
            
        #print tokens



"""

* memory usage
* num allocations
* avg time


fmt:
jc_voronoi                  used 8371052 bytes in 497 allocations
jc_voronoi                  iterations: 1    avg: 51.633 ms    median: 51.633 ms    min: 51.633 ms    max: 51.633 ms

"""

def collect_table_data(counts, report, tabledata):
    for category, results in report.iteritems():
        if not category in tabledata:
            tabledata[category] = OrderedDict()
        
        if not 'counts' in tabledata[category]:
            tabledata[category]['counts'] = list()
        tabledata[category]['counts'].extend(counts)
        
        for name, values in results.iteritems():
            if not name in tabledata[category]:
                tabledata[category][name] = list()
            if name in ['title', 'scale', 'unit']:
                tabledata[category][name] = values
                continue
            tabledata[category][name].extend(values)
            

def make_table_report(data):
    for category, results in data.iteritems():
        columns = list()
        for name, values in results.iteritems():
            if name in ['title', 'scale', 'formatter', 'unit']:
                continue
            columns.append( [name]+values )
        
        formatter = results['formatter']
        scale = results['scale']
        title = results['title']
        
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
                
        print title
        print '-' * len(title)
        print ""
        print '|' + '|'.join(headers) + '|'
        print '|' + '|'.join(headersunderline) + '|'

        for row in rows[1:]:
            values = []
            for ic, v in enumerate(row):
                length = lengths[ic]
                value = v.ljust(length)
                values.append( ' ' + value + ' ')
                
            print '|' + '|'.join(values) + '|'
        
        print ""
        print ""



def make_report_pygal(counts, suffix, report):
    for count in counts:
        run_test( report, count )
        
    for category, results in report.iteritems():
        
        chart = pygal.Bar()
        chart.x_labels = map(str, counts)
        chart.title = results['title']
        
        unit = results['unit']
        chart.value_formatter = lambda x: '%.2f %s' % ((x if x is not None else ''), unit)
        
        scale = results['scale']
        
        for name, values in results.iteritems():
            if name in ['title', 'scale', 'unit']:
                continue

            chart.add(name, map(lambda x: x * scale, values))
            
        chart.render()
        outpath = '../images/%s%s.svg' % (category, suffix)
        chart.render_to_file(outpath)
        outpath = '../images/%s%s.png' % (category, suffix)
        chart.render_to_png(outpath)
        print "Wrote", outpath

def make_report_plotly(counts, suffix, report):
    
    for count in counts:
        run_test( report, count )
        
    for category, results in report.iteritems():
        
        #formatter = results['formatter']
        scale = results['scale']
        unit = results['unit']
        
        bars = []
        for name, values in results.iteritems():
            if name in ['title', 'scale', 'unit']:
                continue

            #values = map(formatter, map(lambda x: x * scale, values))
            values = map(lambda x: x * scale, values)

            bar = Bar(  x=counts,
                        y=values,
                        name=name )
            bars.append(bar)
    
        layout = Layout(title=results['title'],
                        font=Font(family='Raleway, sans-serif'),
                        showlegend=True,
                        barmode='group',
                        bargap=0.15,
                        bargroupgap=0.1,
                        legend=Legend(x=0, y=1.0),
                        xaxis=XAxis(title='Num Sites', type='category'),
                        yaxis=YAxis(title=results['unit'])
                        )
        
        data = Data(bars)
        fig = Figure(data=data, layout=layout)
        outpath = '../images/%s%s.png' % (category, suffix)
        py.image.save_as(fig, outpath)
        
        print "Wrote", outpath
        


if __name__ == '__main__':
    
    counts = [3, 10, 50, 100, 200]
    
    tabledata = OrderedDict()
    tabledata['timings'] = OrderedDict()
    tabledata['memory'] = OrderedDict()
    tabledata['allocations'] = OrderedDict()
    
    report = OrderedDict()
    report['timings'] = OrderedDict()
    report['memory'] = OrderedDict()
    report['allocations'] = OrderedDict()
    
    report['timings']['title'] = 'Timings (microseconds)'
    report['timings']['scale'] = 1000000.0
    report['timings']['unit']  = 'us'
    report['memory']['title'] = 'Memory (kb)'
    report['memory']['scale'] = 1 / 1024.0
    report['memory']['unit']  = 'kb'
    report['allocations']['title'] = '# Allocations'
    report['allocations']['scale'] = 1
    report['allocations']['unit']  = ''
    
    make_report_plotly(counts, '_small', report)
    collect_table_data(counts, report, tabledata)

    counts = [1000, 2000, 5000, 10000, 20000]
    report = OrderedDict()
    report['timings'] = OrderedDict()
    report['memory'] = OrderedDict()
    report['allocations'] = OrderedDict()
    
    report['timings']['title'] = 'Timings (milliseconds)'
    report['timings']['scale'] = 1000.0
    report['timings']['unit']  = 'ms'
    report['memory']['title'] = 'Memory (kb)'
    report['memory']['scale'] = 1 / 1024.0
    report['memory']['unit']  = 'kb'
    report['allocations']['title'] = '# Allocations'
    report['allocations']['scale'] = 1
    report['allocations']['unit']  = ''
    
    make_report_plotly(counts, '_large', report)
    collect_table_data(counts, report, tabledata)
    
    tabledata['timings']['title'] = 'Timings'
    tabledata['timings']['scale'] = 1000.0
    tabledata['timings']['formatter'] = lambda x: '%.4f ms' % x
    tabledata['memory']['title'] = 'Memory'
    tabledata['memory']['scale'] = 1 / 1024.0
    tabledata['memory']['formatter']  = lambda x: '%d kb' % x
    tabledata['allocations']['title'] = '# Allocations'
    tabledata['allocations']['scale'] = 1
    tabledata['allocations']['formatter'] = lambda x: str(x)
    make_table_report(tabledata)
    