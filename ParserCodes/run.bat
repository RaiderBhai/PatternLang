@echo off

python ll1_analyzer.py
python shift_reduce_analyzer.py
python RD_analyzer.py
python lr0_analyzer.py
python slr_analyzer.py
python lalr_analyzer.py
python clr_analyzer.py

echo All scripts finished!
pause
