#!/usr/bin/python
import subprocess
import time
import curses
from curses import wrapper


def main(stdscr):
    height, width = stdscr.getmaxyx()
    curses.start_color()
    curses.use_default_colors()
    for i in range(0, curses.COLORS):
        curses.init_pair(i + 1, i, -1)

    first_line = subprocess.check_output(["head", "-1", "out.txt"])
    destinations = first_line.split(",")

    stdscr.addstr(0, 0, "Andre's Guidance System", curses.A_BOLD)
    stdscr.addstr(2, 0, "Dests Reached: [{}]".format(" "*len(destinations)))
    stdscr.addstr(3, 0, "Going to: ")
    stdscr.addstr(4, 0, "Lat Lon: ")
    stdscr.addstr(5, 0, "GPS Altitude: ")
    stdscr.addstr(6, 0, "GPS Speed: ")
    stdscr.addstr(7, 0, "GPS Heading: ")
    stdscr.addstr(8, 0, "Bearing: ")
    stdscr.addstr(9, 0, "IMU Heading: ")
    stdscr.addstr(10, 0, "Pitch: ")
    stdscr.addstr(11, 0, "Roll: ")
    stdscr.addstr(12, 0, "Flight Time: ")

    while(1):
        last_line = subprocess.check_output(["tail", "-2", "out.txt"]).split("\n")[0]
        entry = last_line.split(',')

        for i in range(3, 13):
             stdscr.addstr(i, 16, "{}".format(" "*10))
        stdscr.addstr(13, 0, "{}".format(" "*10))

        if(entry[0] == 0):
            stdscr.clear()
            stdscr.addstr(0, 0, "Reached Destinations/Guidance Terminated")
        else:
            if(entry[1] == 1):
                stdscr.addstr(13, 0, "NO GPS FIX", curses.color_pair(2))

            stdscr.addstr(2, 16, "{}".format("#"*int(entry[2])), curses.color_pair(3))
            stdscr.addstr(3, 15, "{}".format(destinations[int(entry[2])]), curses.color_pair(4))
            stdscr.addstr(4, 15, "{} {}".format(entry[3], entry[4]), curses.color_pair(3))
            stdscr.addstr(5, 15, "{}m".format(entry[5]), curses.color_pair(3))
            stdscr.addstr(6, 15, "{}kn".format(entry[6]), curses.color_pair(3))
            stdscr.addstr(7, 15, "{}".format(entry[7]), curses.color_pair(3))
            stdscr.addstr(8, 15, "{}".format(entry[8]), curses.color_pair(5))
            stdscr.addstr(9, 15, "{}".format(entry[9]), curses.color_pair(7))
            stdscr.addstr(10, 15, "{}".format(entry[10]), curses.color_pair(7))
            stdscr.addstr(11, 15, "{}".format(entry[11]), curses.color_pair(7))
            stdscr.addstr(12, 15, "{}s".format(entry[12]), curses.color_pair(8))
        
        stdscr.addstr(height-1, 0, "")
        stdscr.refresh()
#        time.sleep(.1)
try:
    wrapper(main)
except(KeyboardInterrupt):
    print("Done")
