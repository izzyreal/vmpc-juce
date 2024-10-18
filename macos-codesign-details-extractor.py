import subprocess
import argparse
import sys


class MyParser(argparse.ArgumentParser):
    def error(self, message):
        sys.stderr.write('error: %s\n' % message)
        self.print_help()
        sys.exit(3)


parser = MyParser(description='Return the first valid Apple Development organisational unit found in your macOS keychain')
args = parser.parse_args()

first_cert = subprocess.check_output("security find-certificate -c \"Apple Development\" -p | /usr/bin/openssl x509 -subject", universal_newlines=True, shell=True, stderr=subprocess.STDOUT)

begin = "/OU="

begin_index = first_cert.find(begin)

if begin == -1:
    raise Exception("Failed to find organisational unit when looking for the following "
                    "string in the output of 'security find-certificate -c \"Apple Development\" -p':", begin)

end = "/O"

end_index = first_cert.find(end, begin_index + len(begin))

if end_index == -1:
    raise Exception("Only organisational unit start marker '\\OU=' was found. " +
                    "Failed to find end marker '/O'  in what seems to be " +
                    "a string with valid certificate info:", first_cert)

organisational_unit = first_cert[begin_index + len(begin):end_index]

sys.stdout.write(organisational_unit)
