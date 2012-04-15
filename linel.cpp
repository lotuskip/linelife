// See linelife.html and/or linelife.1
#include <string>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <set>
#include <fstream>
#ifndef NO_INTERFACE
#include <vector>
#include <ncurses.h>
#include <sys/time.h>
#include <cstdarg>
#endif
using namespace std;

const int VERSION = 3;

#define ALIVE '1'
#define DEAD '0'
const char cell_symbols[] = { DEAD, ALIVE, '\0' };
const char inv_str[] = "Given string contained illegal characters.";
const char str_inv_inv[] = "Invalid invocation!";

#ifndef NO_INTERFACE
const char PLAYBACK_TIMEOUT = 50; // ms to block on getch()
const char tmp_file_name[] = "/tmp/ll";
#endif

/* The internal representation of a sequence of cells is set<int> that contains
 * all the alive cells (the cells are numbered so that the starting position is
 * cell 0, its left-side neighbour is cell -1, its right-side neighbour is cell
 * 1, etc). */
set<int> alives;

char sym_at(const int p)
{
	return cell_symbols[(alives.find(p) != alives.end())];
}

void print_to(ostream &os)
{
	if(alives.empty())
		os << "[extinction]";
	else
	{
		for(int A = *(alives.begin()); A <= *(--(alives.end())); ++A)
			os << sym_at(A);
	}
	os << endl;
}


// position of leftmost living cell in the initial pattern; 0 if none
int init_pos = 0;


// NOTE: these two assume !alives.empty()
double get_density()
{
	return double(alives.size())/(*(--alives.end())-*(alives.begin())+1);
}
int get_shift()
{
	return *(alives.begin()) - init_pos;
}


#ifndef NO_INTERFACE
int scr_x;

void update_scr_x()
{
	int tmp; // we don't need y, but must put it somewhere
	getmaxyx(stdscr, tmp, scr_x);
}

void ll_refresh()
{
	move(0, scr_x/3);
	refresh();
}

// "cursor" position on the line (cursor position _on screen_ is always == scr_x/3)
int pos = 0;
int sel_start_pos; // pos where selection mode was started
bool selectmode = false;

void print_entire_line()
{
	int x;

	move(0,0); // The line itself:
	if(selectmode)
	{
		for(x = pos - scr_x/3; x < min(pos, sel_start_pos); ++x)
			addch(sym_at(x));
		attron(A_STANDOUT);
		for(; x <= max(pos, sel_start_pos); ++x)
			addch(sym_at(x));
		attroff(A_STANDOUT);
		for(; x < pos + 2*scr_x/3; ++x)
			addch(sym_at(x));
	}
	else
	{
		for(x = pos - scr_x/3; x < pos + 2*scr_x/3; ++x)
			addch(sym_at(x));
	}

	move(1,0); // The "ruler":
	for(x = pos - scr_x/3; x < pos + 2*scr_x/3; ++x)
	{
		if(!(x%10))
			addch('|');
		else
			addch(' ');
	}

	ll_refresh();
}


void print_info(const char* fmt, ...)
{
	va_list ap;
	char buf[128];
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	mvaddstr(2, 0, buf);
	clrtoeol();
	ll_refresh();
}


string selection;

void do_yank()
{
	selection.clear();
	for(int j = min(sel_start_pos, pos); j <= max(sel_start_pos, pos); ++j)
		selection += sym_at(j);
}


char type_dir = 1; // +1 for right, -1 for left

void paste()
{
	for(unsigned int j = 0; j < selection.size(); ++j)
	{
		if(selection[j] == ALIVE)
			alives.insert(pos + type_dir*j);
		else
			alives.erase(pos + type_dir*j);
	}
	pos += type_dir*(signed int)(selection.size());
	print_entire_line();
}


char playback = 0; // playback speed; 0 means no playback

void stop_playback()
{
	if(playback)
	{
		playback = 0;
		timeout(-1);
		print_info("Stopped.");
	}
}


void typemove()
{
	stop_playback();
	pos += type_dir;
	print_entire_line();
}


string get_input_str(const char *query)
{
	char buffer[100];
	int rv;
	print_info(query);
	echo();
	rv = mvgetnstr(2, strlen(query), buffer, 100);
	noecho();
	if(rv != ERR)
		return string(buffer);
	return "";
}
#endif


/* The state of the system on the previous step depending on whether
 * we have history or just the previous step: */
#ifdef NO_INTERFACE
set<int> previous_step;
#define PREVIOUS_STEP previous_step
#else
vector< set<int> > history;
#define PREVIOUS_STEP history.back()
#endif


/* Run a step. This is where the Lifeline rules are implemented.
 * Returns true if a static state has been reached. */
bool step(const bool dohistory = true)
{
	if(alives.empty())
		return true;

#ifndef NO_INTERFACE
	if(dohistory || history.empty())
		history.push_back(alives);
#endif

	char n = 1; // first position to check has a single living neighbour
	bool equal = true; // has the step made a change?
	/* NOTE: with the standard rules, we could optimise a little here, but this
	 * algorithm will work correctly with any ruleset given in the Makefile */
	for(int j = *(--(PREVIOUS_STEP.end()))+4; j >= *PREVIOUS_STEP.begin()-4; --j)
	{
		if(PREVIOUS_STEP.find(j) != PREVIOUS_STEP.end()) // cell is alive
		{
			--n; // do not count the cell itself...
			if(DIERULE) // defined in the Makefile!
			{
				alives.erase(j);
				equal = false;
			}
			++n; // ...but do count it in the the next step again
		}
		else // cell is currently dead
			if(BIRTHRULE) // defined in the Makefile!
		{
			alives.insert(j);
			equal = false;
		}
		// update n for the next cell:
		if(PREVIOUS_STEP.find(j-5) != PREVIOUS_STEP.end())
			++n; // one more living cell in next span
		if(PREVIOUS_STEP.find(j+4) != PREVIOUS_STEP.end())
			--n; // and one less
	}
	if(!dohistory // Not doing history; but still record previous step.
		&& !equal) // Avoid unnecessary copying.
		PREVIOUS_STEP = alives;
	return equal;
}

#ifndef NO_INTERFACE
void back(const int gen)
{
	vector< set<int> >::iterator it = history.begin()+gen;
	alives = *it;
	history.erase(it, history.end());
	print_entire_line();
}


void print_gen()
{
	print_info("Generation=%i", history.size());
}
#endif


// Read a pattern in string form; returns true if it is invalid
bool init_from_string(const string &s)
{
	alives.clear();
	if(s.find_first_not_of(cell_symbols) == string::npos)
	{
		for(unsigned int j = 0; j < s.size(); ++j)
		{
			if(s[j] == ALIVE)
				alives.insert(j);
		}
		if(!alives.empty())
		{
			init_pos = *(alives.begin());
#ifndef NO_INTERFACE
			pos = (*(--alives.end())+*(alives.begin()))/2;
		}
#else
		}
		previous_step = alives;
#endif
		return false;
	}
	return true;
}


bool print_steps = false;

void run_independent(const string &orig, const int steps)
{
	int j;
	for(j = 0; j < steps; ++j)
	{
		if(print_steps)
		{
			cout << "gen " << j;
			if(!alives.empty())
				cout << " (dens " << get_density() << ", shift "
					<< get_shift() << ')';
			cout << ": ";
			print_to(cout);
		}
		if(step(false)) // step without history record
			break;
	}
	cout << orig << " after " << j << " generations (shift "
		<< get_shift() << "): ";
	print_to(cout);
}


int main(int argc, char* argv[])
{
	char inited = 0; // 1: succesfully; -1: error
	int j, input = 0, batch_index = 0;
	string s;
	for(j = 1; j < argc; ++j)
	{
		if(!strcmp(argv[j], "-h") || !strcmp(argv[j], "-v"))
		{
			cout << "Linelife " << VERSION <<
#ifdef NO_INTERFACE
				" (no-interface build)" <<
#endif
#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)
				endl << "with dierule \"" STR(DIERULE) "\" and birthrule \""
				STR(BIRTHRULE) "\"" <<
				endl << "Please see the linelife.html file and the man page for information."
					<< endl;
			return 0;
		}
		else if(!strcmp(argv[j], "-s"))
			print_steps = true;
		else if(!strcmp(argv[j], "-b"))
		{
			if((batch_index = ++j) == argc)
			{
				cerr << str_inv_inv << endl;
				return 1;
			}
		}
		else if(!strcmp(argv[j], "-n"))
		{
			char* chp;
			if(++j == argc
				|| (input = strtol(argv[j], &chp, 10)) <= 0 || *chp != '\0')
			{
				cerr << str_inv_inv << endl;
				return 1;
			}
			// else input now holds the number of generations to run
		}
		// assume any other argument is the initial pattern
		else if(init_from_string((s = argv[j])))
			inited = -1;
		else
			inited = 1;
	}

	if(batch_index) // running a batch job
	{
		if(input <= 0)
		{
			cerr << "No valid number of steps given." << endl;
			return 1;
		}
		ifstream file(argv[batch_index]);
		if(!file)
		{
			cerr << "Could not open input file \'" << argv[batch_index] << '\'' << endl;
			return 1;
		}
		while(getline(file, s))
		{
#ifndef NO_INTERFACE
			history.clear();
#endif
			if(init_from_string(s))
				cerr << "Warning: invalid pattern read!" << endl;
			else
				run_independent(s, input);
		}
		return 0;
	}

	if(input > 0) // running a single run independently
	{
		// Check to read starting pattern from stdin
		if(!inited)
		{
			cin >> s;
			if(!init_from_string(s))
				inited = 1;
		}
		if(inited <= 0)
		{
			cerr << inv_str << endl;
			return 1;
		}
		run_independent(s, input);
		return 0;
	}
	// else
	
#ifdef NO_INTERFACE
	cerr << "This version of Linelife does not contain the interface." << endl
		<< "Only automatic mode is available!" << endl;
	return 1;
#else
	// Init Curses:
	if(!initscr())
	{
		cout << "Failed to init Curses screen!" << endl;
		return 1;
	}
	cbreak();
	keypad(stdscr, TRUE);
	noecho();
	nonl();
	set_escdelay(20); //argument is ms
	refresh();
	flushinp();

	update_scr_x();
	print_entire_line();
	if(inited == -1)
		print_info(inv_str);

	char playback_to_resume = 1;
	int multiplier = 1;
	timeval last, now;
	gettimeofday(&last, 0);
	set<int>::iterator bi;

	for(;;)
	{
		if(playback) // playback mode; do steps automatically
		{
			// Get time from last update
			gettimeofday(&now, 0);
			if((now.tv_sec - last.tv_sec)*1000 + (now.tv_usec - last.tv_usec)/1000
				>= 1200 - 200*playback)
			{
				if(step())
				{
					print_info("Static state; stopping playback.");
					playback = 0;
				}
				print_entire_line();
				gettimeofday(&last, 0);
			}
		}

		input = getch();
		// Global keys:
		if(input == KEY_RESIZE || (input == ERR && !playback))
		{
			// these values usually indicate that the terminal was resized
			update_scr_x();
			print_entire_line();
		}
		else if(input == 'h' || input == KEY_LEFT) // scroll left
		{
			pos -= multiplier;
			if(!playback)
				print_entire_line();
		}
		else if(input == 'l' || input == KEY_RIGHT) // scroll right
		{
			pos += multiplier;
			if(!playback)
				print_entire_line();
		}
		else if(input == 'j' || input == KEY_DOWN) // decrease step
		{
			if(multiplier > 1)
				multiplier /= 10;
			print_info("Multiplier=%i", multiplier);
		}
		else if(input == 'k' || input == KEY_UP) // increase step
		{
			if(multiplier < 100000)
				multiplier *= 10;
			print_info("Multiplier=%i", multiplier);
		}
		else if(input == 'q') // quit
			break;
		else if(input == 'r') // reset
		{
			if(!alives.empty())
			{
				pos = (*(--alives.end())+*(alives.begin()))/2;
				if(!playback)
					print_entire_line();
			}
			else
				print_info("No population.");
		}
		else if(input == 's')
			print_info("Sel: %s", selection.c_str());
		else if(input == 't')
			print_info("Pos: %i", pos);
		// Select mode keys:
		else if(selectmode)
		{
			switch(input)
			{
			case '\t': case 27: //escape
				selectmode = false;
				print_entire_line();
				print_info("");
				break;
			case 'd': // delete selection
			{
				selectmode = false;
				do_yank();
				// delete the range:
				alives.erase(alives.lower_bound(min(sel_start_pos, pos)),
					alives.upper_bound(max(sel_start_pos, pos)));
				alives.erase(max(sel_start_pos, pos));
				// move back everything after the selection
				bi = alives.lower_bound(max(sel_start_pos, pos));
				set<int> tmpset(bi, alives.end());
				alives.erase(bi, alives.end());
				for(bi = tmpset.begin(); bi != tmpset.end(); ++bi)
					alives.insert(*bi - selection.size());
				print_entire_line();
				print_info("Selection cut.");
				break;
			}
			case 'y': // yank selection
				selectmode = false;
				do_yank();
				print_entire_line();
				print_info("Selection copied.");
				break;
			case ALIVE: // set all alive
				selectmode = false;
				for(j = min(sel_start_pos, pos); j <= max(sel_start_pos, pos); ++j)
					alives.insert(j);
				print_entire_line();
				break;
			case DEAD: // set dead
				selectmode = false;
				for(j = min(sel_start_pos, pos); j <= max(sel_start_pos, pos); ++j)
					alives.erase(j);
				print_entire_line();
				break;
			}
		} // selectmode
		else // normal mode keys:
		{
			switch(input)
			{
			case 'n': // print population
				stop_playback();
				if(alives.empty())
					print_info("No population.");		
				else
					print_info("pop %i, len: %i, dens: %f, shift: %i",
						alives.size(), *(--alives.end())-*(alives.begin())+1,
						get_density(), get_shift());
				break;
			case 'g': // print generation
				stop_playback();
				print_gen();
				break;
			case 'G': // goto generation
				stop_playback();
				print_info("Enter generation: ");
				echo();
				mvscanw(2, 18, (char*)"%d", &j);
				noecho();
				if(j < 0)
					print_info("Cannot go to a negative generation!");
				else if((unsigned int)j < history.size())
				{
					back(j);
					print_gen();
				}
				else // it is in the future
				{
					for(; j > 0; --j)
					{
						if(step())
							break; // no point iterating; it's a static state!
					}
					print_entire_line();
					if(!j)
						print_gen();
					else
						print_info("Static state reached on gen. %i", history.size());
				}
				break;
			case 'm': // step forward
				stop_playback();
				if(step())
				{
					if(alives.empty())
						print_info("Extinction.");
					else
						print_info("Static state!");
				}
				else
					print_gen();
				print_entire_line();
				break;
			case 'b': // step backward
				stop_playback();
				if(history.empty())
					print_info("No history.");
				else
				{
					back(history.size()-1);
					print_gen();
				}
				break;
			case 'x': // exchange two cells
				if(alives.find(pos) == alives.end())
				{
					if(alives.find(pos+type_dir) != alives.end())
					{
						alives.insert(pos);
						alives.erase(pos+type_dir);
					}
				}
				else if(alives.find(pos+type_dir) == alives.end())
				{
					alives.erase(pos);
					alives.insert(pos+type_dir);
				}
				typemove();
				break;
			case 'P': // paste (overwriting)
				stop_playback();
				if(!selection.empty())
				{
					paste();
					print_info("Pasted.");
				}
				else
					print_info("No selection.");
				break;
			case 'p': // paste (inserting)
			{
				stop_playback();
				if(!selection.empty())
				{ 
					// move forward all alive cells that will be after the inserted bit:
					bi = alives.upper_bound(pos);
					set<int> tmpset(bi, alives.end());
					alives.erase(bi, alives.end());
					for(bi = tmpset.begin(); bi != tmpset.end(); ++bi)
						alives.insert(*bi + selection.size());
					// Then paste as above, but after the cursor
					++pos;
					paste();
					print_info("Inserted.");
				}
				else
					print_info("No selection.");
				break;
			}
			case 'v': // go to selection mode
				stop_playback();
				selectmode = true;
				sel_start_pos = pos;
				print_info("Selection mode");
				attron(A_STANDOUT);
				addch(sym_at(pos));
				attroff(A_STANDOUT);
				ll_refresh();
				break;
			case ALIVE: // set alive
				alives.insert(pos);
				typemove(); // this calls stop_playback()!
				break;
			case DEAD: // set dead
				alives.erase(pos);
				typemove(); // this calls stop_playback()!
				break;
			case 'T': // change "typing direction"
				stop_playback();
				if((type_dir *= -1) == 1)
					print_info("Now typing to the right.");
				else print_info("Now typing to the left.");
				break;
			case ' ': // toggle
				stop_playback();
				if(alives.find(pos) != alives.end())
				{
					alives.erase(pos);
					addch(DEAD);
				}
				else
				{
					alives.insert(pos);
					addch(ALIVE);
				}
				ll_refresh();
				break;
			case 'S': // save
			{
				stop_playback();
				if(alives.empty())
				{
					print_info("There is nothing to save!");
					break;
				}
				ofstream file(get_input_str("Enter filename: ").c_str());
				if(!file)
				{
					print_info("Could not open that file for writing.");
					break;
				}
				print_to(file);
				print_info("Save successful!");
				break;
			}
			case 'O': // open
			{
				stop_playback();
				ifstream file(get_input_str("File to load: ").c_str());
				if(!file)
				{
					print_info("Could not open that file for reading.");
					break;
				}
				getline(file, s);
				if(init_from_string(s))
					print_info("Could not read a valid pattern.");
				else print_info("Loaded.");
				print_entire_line();
				history.clear();
				break;
			}
			case 'E': // erase history
				history.clear();
				print_info("History cleared!");
				break;
			case '>': // speed up playback
				if(!playback)
					timeout(PLAYBACK_TIMEOUT);
				if(playback < 5)
					++playback;
				print_info("Playback at %i", int(playback));
				break;
			case '<': // slow down playback
				if(playback)
				{
					if(!(--playback))
						stop_playback();
					else
						print_info("Playback at %i", int(playback));
				}
				break;
			case '.': // stop playback
				stop_playback();
				playback_to_resume = 1;
				break;
			case ',': // toggle pause on playback
				if(playback)
				{
					playback_to_resume = playback;
					stop_playback();
					print_info("Paused.");
				}
				else
				{
					playback = playback_to_resume;
					print_info("Playback at %i", int(playback));
					timeout(PLAYBACK_TIMEOUT);
				}
				break;
			case ':':
				if(!(s = get_input_str(":")).empty())
				{
					// Save to tmp, make system call, load back up:
					fstream file(tmp_file_name, ios_base::out);
					if(!file)
					{
						print_info("Error opening temp file.");
						break;
					}
					print_to(file);
					file.close();
					system(s.c_str());
					file.open(tmp_file_name, ios_base::in);
					getline(file, s);
					init_from_string(s);
					print_entire_line();
				}
				break;
			} // switch input
		} // normal mode
	} // forever

	endwin();
	return 0;
#endif
}
