#!/bin/bash
set -e
dot -Tpdf -o project.pdf project.dot
pdflatex main.tex
