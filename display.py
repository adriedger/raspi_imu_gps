import subprocess
import time
import curses
from curses import wrapper


def main(stdscr):
    height, width = stdscr.getmaxyx()
    curses.init_pair(1, curses.COLOR_RED, curses.COLOR_BLACK)
    curses.init_pair(2, curses.COLOR_BLUE, curses.COLOR_BLACK)
    curses.init_pair(3, curses.COLOR_GREEN, curses.COLOR_BLACK)
    curses.init_pair(4, curses.COLOR_YELLOW, curses.COLOR_BLACK)
    curses.init_pair(5, curses.COLOR_CYAN, curses.COLOR_BLACK)
    curses.init_pair(6, curses.COLOR_MAGENTA, curses.COLOR_BLACK)

    stdscr.addstr(0, 0, "Andre's Avionics System") 
#    stdscr.addstr(1, 0, "Coordiantes are in degrees decimal") 

    first_line = subprocess.check_output(["head", "-1", "out.txt"])
    destinations = first_line.split(",")

    while(1):
        last_line = subprocess.check_output(["tail", "-2", "out.txt"]).split("\n")[0]
        entry = last_line.split(',')
        if(entry[0] == 0):
            stdscr.clear()
            stdscr.addstr(0, 0, "Reached Destinations/Guidance Terminated")
        else: 
            if(entry[1] == 1):
                stdscr.addstr(13, 0, "NO GPS FIX", curses.color_pair(1), curses.A_BLINK)

            stdscr.addstr(2, 0, "Going to: {}".format(destinations[int(entry[2])]), curses.color_pair(6))
            stdscr.addstr(3, 0, "Latitude: {}".format(entry[3]), curses.color_pair(3))
            stdscr.addstr(4, 0, "Longitude: {}".format(entry[4]), curses.color_pair(3))
            stdscr.addstr(5, 0, "GPS Altitude: {}m".format(entry[5]), curses.color_pair(3))
            stdscr.addstr(6, 0, "GPS Speed: {}kn".format(entry[6]), curses.color_pair(3))
            stdscr.addstr(7, 0, "GPS Heading: {}".format(entry[7]), curses.color_pair(3))
            stdscr.addstr(8, 0, "Bearing: {}".format(entry[8]), curses.color_pair(2))
            stdscr.addstr(9, 0, "IMU Heading: {}".format(entry[9]), curses.color_pair(5))
            stdscr.addstr(10, 0, "Pitch: {}".format(entry[10]), curses.color_pair(5))
            stdscr.addstr(11, 0, "Roll: {}".format(entry[11]), curses.color_pair(5))
            stdscr.refresh()
#        time.sleep(.1)
try:
    wrapper(main)
except(KeyboardInterrupt):
    print("Done")
