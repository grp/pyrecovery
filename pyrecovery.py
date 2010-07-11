from iphone import Device
import sys, os, readline
from optparse import OptionParser
from StringIO import StringIO

parser = OptionParser()
parser.add_option("-f", "--file", dest="file", help="Send file to client.")
parser.add_option("-c", "--command", dest="cmd", help="Send command to client.")
parser.add_option("-e", "--execute", dest="script", help="Executes recovery shell script.")
parser.add_option("-s", "--shell", action="store_true", dest="shell", help="Start interactive shell.")
parser.add_option("-r", "--reset", action="store_true", dest="reset", help="Reset client.")
(options, args) = parser.parse_args()

if bool(options.reset) + bool(options.shell) + bool(options.script) + bool(options.file) + bool(options.cmd) > 1:
	parser.error("Conflicting options specified.")

device = Device()
device.connect()

if options.reset:
	device.reset()
elif options.file:
	data = open(options.file, 'rb').read()
	device.send_file(data)
elif options.cmd:
	device.send_command(options.cmd)
else:
	if options.script:
		file = options.file
	elif options.shell:
		print ">> Welcome pyRecovery by chpwn <<"
		file = sys.stdin

	while True:
		if file == sys.stdin: 	line = raw_input("> ")
		else: 					line = file.readline()
	
		if line == "exit":
			break
		elif line.startswith("getenv"):
			var = line[line.find(" ") + 1:]
			print device.getenv(var)
		elif line.startswith("info"):
			var = line[line.find(" ") + 1:]
			print device.getinfo(var) # "ECID", "IMEI", etc
		else:
			device.send_command(line)
			print device.receive()

device.disconnect()
