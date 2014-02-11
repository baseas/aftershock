import sys
import getopt
import configparser

def usage():
    print("usage: cvartool -f -o")

def main(argv):
    try:
        opts, args = getopt.getopt(argv, '-fo', ['format', 'output'])
    except getopt.GetoptError:
        usage()
        return

    outformat = None
    outfile = None

    for opt, arg in opts:
        print(arg)
        if opt in ('-f', '--format'):
            if arg not in ('html', 'man', 'plain'):
                print('Possible formats are html, man, plain.')
            outformat = arg
            return
        elif opt in ('-o', '--output'):
            outfile = arg

    if outformat == None:
        print("No format specified.")
        return
    elif outfile == None:
        print("No output file specified.")
        return

    parseConfig(outformat, outfile)

def outputHtml(config):
    print('<dl>')
    for section in config.sections():
        print('    <dt>%s</dt>' % section)
        print('    <dd>%s</dd>' % config[section]['doc'])
    print('</dl>')

def outputMan(config):
    pass

def parseConfig(outformat, outfile):
    config = configparser.ConfigParser()
    config.read('cvarlist.ini')

    if outformat == 'html':
        output = outputHtml(config)
    elif outformat == 'man':
        output = outputMan(config)

    if outfile is None:
        print(output)
        return

    fp = open(outfile, 'w')
    fwrite(fp, output)
    fclose(fp)

if __name__ == '__main__':
    main(sys.argv[1:])

