#! /usr/bin/python

import sys
import os
from collections import OrderedDict
import json


# If set to true, the output from the actual runs are printed out to the stdout
NOPRESENT='-v' in sys.argv

class Test(object):
    def __init__(self, implementation):
        self.implementation = implementation
        self.results = []

"""
tests = OrderedDict()

tests['Factorial (recursive)'] = (  [OrderedDict([('n',40)]), OrderedDict([('n',100)]), OrderedDict([('n',200)]), OrderedDict([('n',400)]), OrderedDict([('n',600)]), OrderedDict([('n',800)]) ],
                                    [ Test('C++', BUILDDIR+'/fact_recursive'),
                                       Test('Python 2', 'python python/fact_recursive.py'),
                                       Test('Python 3', 'python3 python/fact_recursive.py'),
                                       Test('C#', CSHARPINTERPRETER + BUILDDIR+'/fact_recursive.exe')] )

tests['Factorial (loop)'] = ( [OrderedDict([('n',40)]), OrderedDict([('n',100)]), OrderedDict([('n',200)]), OrderedDict([('n',400)]), OrderedDict([('n',600)]), OrderedDict([('n',800)]) ],
                                [Test('C++', BUILDDIR+'/fact_loop'),
                                  Test('Python 2', 'python python/fact_loop.py'),
                                  Test('Python 3', 'python3 python/fact_loop.py'),
                                  Test('C#', CSHARPINTERPRETER + BUILDDIR+'/fact_loop.exe')] )

tests['Fibonacci (recursive)'] = (  [OrderedDict([('n',4)]), OrderedDict([('n',8)]), OrderedDict([('n',12)]), OrderedDict([('n',16)]), OrderedDict([('n',20)]) ],
                                    [ Test('C++', BUILDDIR+'/fib_recursive'),
                                      Test('Python 2', 'python python/fib_recursive.py'),
                                      Test('Python 3', 'python3 python/fib_recursive.py'),
                                      Test('C#', CSHARPINTERPRETER + BUILDDIR+'/fib_recursive.exe')
                                    ])

tests['Fibonacci (loop)'] = (  [OrderedDict([('n',25)]), OrderedDict([('n',50)]), OrderedDict([('n',75)]), OrderedDict([('n',100)]), OrderedDict([('n',250)]), OrderedDict([('n',500)]) ],
                                    [ Test('C++', BUILDDIR+'/fib_loop'),
                                      Test('Python 2', 'python python/fib_loop.py'),
                                      Test('Python 3', 'python3 python/fib_loop.py'),
                                      Test('C#', CSHARPINTERPRETER + BUILDDIR+'/fib_loop.exe')
                                    ])
"""

def find_time_unit(t):
    if t < 0.000001:
        return 'ns'
    if t < 0.001:
        return '\xb5s'
        #return 'us'
    if t < 0.1:
        return 'ms'
    return 's'

def convert_time(t, unit):
    if unit == 'ns':
        return t * 1000000000.0
    if unit == '\xb5s':
    #if unit == 'us':
        return t * 1000000.0
    if unit == 'ms':
        return t * 1000.0
    return t

def get_average_time(tests):
    t = sum([test.results[-1]['time_avg'] for test in tests])
    return t / len(tests)

def present(groupname, args, tests):
    timeunit = find_time_unit(get_average_time(tests))
    args = '(' + ','.join(['%s = %s' % (k, v) for k, v in args.iteritems()]) + ')'
    
    print groupname, args
    for test in tests:
        print test.implementation.ljust(14), 'average', convert_time(test.results[-1]['time_avg'], timeunit), timeunit
    print ""



def present_html(stream, groupname, argslst, tests):
    
    html = """
    <script type="text/javascript">
      google.load("visualization", "1", {packages:["corechart"]});
      google.setOnLoadCallback(drawChart%(FUNCTIONTITLE)s);
      function drawChart%(FUNCTIONTITLE)s() {
        var data = google.visualization.arrayToDataTable(
            %(TABLEDATA)s
        );

        var options = {
          title: '%(TITLE)s',
          hAxis: {title: '%(HAXIS_TITLE)s'},
          vAxis: {title: '%(VAXIS_TITLE)s'},
          is3D: true,
          backgroundColor: {
            'fill': '#F4F4F4',
            'strokeWidth': 1,
            'opacity': 100
             },
        };

        var chart = new google.visualization.ColumnChart(document.getElementById('chart_div_%(FUNCTIONTITLE)s'));
        chart.draw(data, options);
      }
    </script>
    """
    
    timeunit = find_time_unit(get_average_time(tests))
    
    info = dict()
    info['TITLE'] = groupname
    info['FUNCTIONTITLE'] = ''.join([x for x in groupname if str.isalpha(x)])
    info['VAXIS_TITLE'] = 'Unit: ' + timeunit
    info['HAXIS_TITLE'] = 'Args'
    info['TABLEDATA'] = ''
    
    headers = ['Args']
    for args in argslst:
        headers.append( ','.join(['%s = %s' % (k, v) for k, v in args.iteritems()]))
    
    info['TABLEDATA'] = [headers]
    
    for test in tests:
        langinfo = [test.implementation] + [convert_time(result['time_avg'], timeunit) for result in test.results]
        info['TABLEDATA'].append(langinfo) 

    # transpose the data
    info['TABLEDATA'] = zip(*info['TABLEDATA'])
    info['TABLEDATA'] = map(list, info['TABLEDATA'])
    
    print >>stream, html % info

def present_html_header(stream, alltests):
    
    html = """
<html>
  <head>
    <script type="text/javascript" src="https://www.google.com/jsapi"></script>
    <script>
        function toggle_visibility(id) {
            var e = document.getElementById(id);
            if(e.style.display == 'none')
                e.style.display = 'block';
            else
                e.style.display = 'none';
        }
    </script>
      """
    print >>stream, html

def present_html_footer(stream, alltests):
    html1 = """
      </head>
  <body>
    """
    html2 = """
  </body>
</html>
    """
    
    
    div = """
        <hr />
        <a id="chart_div_anchor_%(FUNCTIONTITLE)s" />
        %(TEXT)s
        <div id="chart_div_%(FUNCTIONTITLE)s" style="width: 600px; height: 350px;"></div>
        <a href="#chart_div_result_%(FUNCTIONTITLE)s" onclick="toggle_visibility('chart_div_result_%(FUNCTIONTITLE)s');">Results:</a>
        <div id="chart_div_result_%(FUNCTIONTITLE)s" style="display: none">
        <pre>
%(RESULT)s
        </pre>
        </div>
    """
    
    link = """<a href="#chart_div_anchor_%(FUNCTIONTITLE)s">%(TITLE)s</a><br>"""
    
    print >>stream, html1
    
    
    for groupname, tests in alltests.iteritems():
        info = dict()
        info['TITLE'] = groupname
        info['FUNCTIONTITLE'] = ''.join([x for x in groupname if str.isalpha(x)])
        
        print >>stream, link % info
    
    print >>stream, "<p/>"
    
    for groupname, tests in alltests.iteritems():
        info = dict()
        info['FUNCTIONTITLE'] = ''.join([x for x in groupname if str.isalpha(x)])
        info['TEXT'] = ''
        
        
        info['RESULT'] = ''
        """
        for i, args in enumerate(argslst):
            info['RESULT'] += ','.join(['%s = %s' % (k, v) for k, v in args.iteritems()]) + ':\n'
            for test in tests:
                info['RESULT'] += ' '*4 + test.implementation.ljust(14) + test.results[i]['result'] + '\n'
            info['RESULT'] += '\n'"""
        print >>stream, div % info
    
    print >>stream, html2


"""

* memory usage
* num allocations
* avg time


fmt:
jc_voronoi                  used 8371052 bytes in 497 allocations
jc_voronoi                  iterations: 1    avg: 51.633 ms    median: 51.633 ms    min: 51.633 ms    max: 51.633 ms

"""

if __name__ == '__main__':
    with open(sys.argv[1], 'rb') as f:
        allresults = json.loads(f.read())
        
    alltests = {}
    for testname, results in allresults.iteritems():
        testname = str(testname)
        for result in results:
            if not 'category' in result:
                continue
            if not testname in alltests:
                alltests[testname] = []
            category = str(result['category'])
            test = Test( category )
            test.results.append(result)
            alltests[testname].append(test)
        
    
    output = sys.stdout
    if len(sys.argv) > 2:
        output = open(sys.argv[2], 'wb')

    present_html_header(output, alltests)
    
    args = { 'n': 5000 }        
    for groupname, tests in alltests.iteritems():
        present(groupname, args, tests)
            
        present_html(output, groupname, [args], tests)
        output.write('\n')

    present_html_footer(output, alltests)
    