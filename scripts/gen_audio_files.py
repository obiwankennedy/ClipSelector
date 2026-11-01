#!/usr/bin/env python3
# -*- coding: utf-8 -*-

""" Generic builder from several images"""

import argparse
import sys
import json
import subprocess

def call_subprocess(args):
    print(args)
    subprocess.call(args, cwd="/home/renaud/documents/11_rolisteam/05_documentation/01_videos/v1.10")


def parse_json(file, dest, lang):

    voices = {"en":"en_US-amy-medium.onnx", "fr":"fr_FR-mls-medium.onnx"}
    voice =  voices[lang]

    with open(file, 'r') as f:
        config = json.load(f)



        i =0
        for text in config:
            output="{}/file_{}.wav".format(dest, str(i).zfill(2))
            i+=1
            args=["/home/renaud/documents/11_rolisteam/05_documentation/01_videos/v1.10/.venv/bin/python3","-m","piper", "-m",voice,"-f",output,"--sentence-silence",'1',"--",text]
            call_subprocess(args)



def main(argv):
    print("Audio Generator v1.0")
    parser = argparse.ArgumentParser(description='Process some integers.')
    parser.add_argument('-t', '--text',dest='text',
                        help='Json file with all texts')
    parser.add_argument('-d', '--destination', dest='dest',
                        help='folder where to put audio')
    parser.add_argument('-l', '--language', dest='lang',
                        help='select a lang: fr or en [default]',choices=["en","fr"],default="en")

    args = parser.parse_args()



    parse_json(args.text, args.dest, args.lang)



if __name__ == '__main__':
    main(sys.argv[1:])
