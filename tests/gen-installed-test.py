import sys
import argparse
import os

template = '''[Test]
Type=session
Exec={}
'''

def build_template(test_dir, test_name):
    return template.format(os.path.join(test_dir, test_name))

if __name__ == '__main__':
    argparser = argparse.ArgumentParser(description='Generate installed-test description file')
    argparser.add_argument('installed_test_dir', help='Path for installed test binaries')
    argparser.add_argument('test_name', help='Name of the test unit')
    argparser.add_argument('out_dir', help='Path for the output')

    args = argparser.parse_args()

    outfile = os.path.join(args.out_dir, args.test_name + '.test')
    with open(outfile, 'w') as f:
        f.write(build_template(args.installed_test_dir, args.test_name))
