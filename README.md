# Code for arduino projects

##  pulse-o-matic
### pulse-o-matic_case.svg
Plans for the case 

### matrix_sweep
* Arduino code to sweep LEDs across a LED matrix like https://www.adafruit.com/product/2279
* Requires Arduino Mega
* Supports three modes changed by a switch:
 * Sweep a single LED through a column then over to the next row, etc.
 * Sweep a whole column of LEDs row-by-row (changing color progressively)
 * Illuminate the whole matrix, changing color progressively
* The speed can be adjusted by a potentiometer
* Also can control a speaker (just a piezo buzzer) with adjustable volume

Used libraries:
* https://github.com/adafruit/Adafruit-GFX-Library
* https://github.com/adafruit/RGB-matrix-Panel

See:

https://git-scm.com/book/en/v2/Git-Tools-Submodules

for info on using submodules
