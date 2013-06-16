#ifndef HEADER_HPP
#define HEADER_HPP

#include <sys/time.h>

#include <cstdio>
#include <string>

#include <sstream>

using namespace std;



void SYS_ERROR(string str){
  perror(str.c_str());
//   exit(1);
}

#define _STR(x) #x
#define STR(x) _STR(x)
#define SYS_ERROR(msg) SYS_ERROR(string( STR(__LINE__) ": " ) + msg)

#define EXIT_CODE_COUNTER (__COUNTER__ + 1)

#define NYI SYS_ERROR("Not yet implemented.");

void mySleep(double czas){
  struct timespec req;
  req.tv_sec = (time_t)floor(czas); /* seconds */
  req.tv_nsec = (long)( ( czas - floor(czas) ) * 1e9 ); /* nanoseconds */
  nanosleep(&req, NULL);
}

double czasRzeczywisty(){
  struct timeval tval;

  if(gettimeofday(&tval, NULL)){SYS_ERROR("gettimeofday error"); exit(EXIT_CODE_COUNTER);}

  return tval.tv_sec + 1e-6*tval.tv_usec;
}

void interrupt(int signo){ exit(0); }

const string bialeZnaki = " \t\r";

const int WIELKOSC_BACKLOGU = 100;

const double Pod = 1.00002406790006336880498073623209268063; // podstawa funkcji wykladniczej wspolczynnika skalujacego wynik od czasu

int deskryptorSocketuAkceptora = -1;
vector<pair<FILE *, string> > handlerySocketowINazwyZespolowPerKlient;

void clean()
{
  if(-1 != deskryptorSocketuAkceptora)
    close(deskryptorSocketuAkceptora);
  for(int i=0;i<handlerySocketowINazwyZespolowPerKlient.size();++i)
    if(NULL != handlerySocketowINazwyZespolowPerKlient[i].first)
      fclose(handlerySocketowINazwyZespolowPerKlient[i].first);
}

template <typename T>
string NumberToString(T Number){
   ostringstream ss;
   ss << Number;
   return ss.str();
}

vector<string> myExplode(string const & explosives, string const & delimeters = bialeZnaki){
  
  vector<string> ret;
  
  size_t pos = 0;
  
  while(string::npos != ( pos = explosives.find_first_not_of(delimeters, pos) )
  ){
    size_t endPos = explosives.find_first_of(delimeters, pos);
    
    ret.push_back(explosives.substr(pos, endPos-pos));
    
    pos = (string::npos == endPos) ? string::npos : endPos+1;
  }
  
  return ret;
}

char hashStringToLetter(const string & str){
  char ret = str[0];
  for(int i=1;i<str.size();++i)
    ret ^= str[i];
  
  return ret%26 + 'a';
}

void sendString(FILE * handler, const string & wiadomosc, bool newline = true){
  fprintf(handler, newline ? "%s\n" : "%s", wiadomosc.c_str());
  fflush(handler);
}

void sendError(FILE * handler, int kodBledu, bool newline = true){
  string msg = "FAILED " + NumberToString(kodBledu) + " ";
  
  switch(kodBledu){
    case 1  : msg += "bad login or password"; break;
    case 2  : msg += "unknown command"; break;
    case 3  : msg += "bad format"; break;
    case 4  : msg += "too many arguments"; break;
    case 5  : msg += "internal error, sorry..."; break;
    case 6  : msg += "commands limit reached, forced waiting activated"; break;
    case 101: msg += "incorrect survivor identifier"; break;
    case 102: msg += "destination is not neighbour"; break;
    case 103: msg += "destination is outside the world"; break;
    case 104: msg += "unavailable - survivor is not on land"; break;
    case 105: msg += "incorrect identifier, the survivor has drown"; break;
    case 106: msg += "captain cannot build raft"; break;
    case 107: msg += "not enough sticks to set wood on fire"; break;
    case 108: msg += "nothing to give"; break;
    case 109: msg += "guard cannot do such operation"; break;
    case 110: msg += "survivor is already a guard"; break;
    case 111: msg += "survivor is busy"; break;
    case 112: msg += "survivor is not a guard"; break;
    case 113: msg += "survivor is already a captain"; break;
    case 114: msg += "survivor is not a captain"; break;
    case 115: msg += "captain cannot guard wood"; break;
    case 116: msg += "cannot take over a raft with a captain"; break;
    case 117: msg += "rafts count limit reached"; break;
    case 118: msg += "no rafts to take over"; break;
    case 119: msg += "you cannot carry wood to become a captain"; break;
    case 120: msg += "wood is burning - taking not possible"; break;
    case 121: msg += "unable to dry non empty raft"; break;
    case 123: msg += "already drying"; break;
    
    default:
      SYS_ERROR("Nieznany kod bledu.");
      return;
  }
  
  if(104 != kodBledu) //TODO usunac ta linie
    cout << "Wysylam blad: \"" << msg << '"' << endl;
  
  sendString(handler, msg, newline);
}

string recieveString(FILE * handler){
  string ret;
  do ret += getc(handler); while('\n' != ret[ret.size()-1]);
  return ret.erase(ret.size()-1);
}

#endif