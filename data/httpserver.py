#!/usr/bin/env python3
# this python code is only here to be able to run an http webserver on the development pc that hosts the html files
# to edit/debug them locally.
# once ready one should use the Project Task "Build Filesystem Image" and "UploadFilesystem Image"
from http.server import HTTPServer, SimpleHTTPRequestHandler, test
import sys

class CORSRequestHandler (SimpleHTTPRequestHandler):
    def end_headers (self):
        self.send_header('Access-Control-Allow-Origin', '*')
        SimpleHTTPRequestHandler.end_headers(self)

print("here we are")
if __name__ == '__main__':
    print("in main")
    test(CORSRequestHandler, HTTPServer, port=int(sys.argv[1]) if len(sys.argv) > 1 else 8000)

print("bye")