#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <climits>

#include <iostream>
#include <fstream>

#include <string>
#include <vector>
#include <map>
#include <set>
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

/////

class Mutex{
  
  pthread_mutex_t mutex;

  Mutex& operator=(const Mutex&){}
  Mutex(const Mutex&){}
  
public:
  
  Mutex(){
    if(pthread_mutex_init(&mutex, NULL)){SYS_ERROR("pthread_mutex_init error"); exit(EXIT_CODE_COUNTER);}
  }
  
  ~Mutex(){
    if(pthread_mutex_destroy(&mutex)){SYS_ERROR("pthread_mutex_destroy error"); exit(EXIT_CODE_COUNTER);}
  }
  
  void lock(){
    if(pthread_mutex_lock(&mutex)){SYS_ERROR("pthread_mutex_lock error"); exit(EXIT_CODE_COUNTER);}
  }
  
  void unlock(){
    if(pthread_mutex_unlock(&mutex)){SYS_ERROR("pthread_mutex_unlock error"); exit(EXIT_CODE_COUNTER);}
  }
};

template<typename T>
class AtomicWrapper{
  
  class PtrWrapper{
    
    T* mPtr;
    Mutex& mMutex;

    //PtrWrapper(const PtrWrapper&&){}
    PtrWrapper& operator=(const PtrWrapper&){}

  public:

    PtrWrapper(const PtrWrapper& copy):
      mPtr(copy.mPtr),
      mMutex(copy.mMutex){}

    PtrWrapper(T* t, Mutex& m): mPtr(t), mMutex(m){
        mMutex.lock();
    }

    ~PtrWrapper(){
        mMutex.unlock();
    }

    T* operator->() const{
        return mPtr;
    }

    T operator*(){
      return *mPtr;
    }
    
    template <typename U>
    U runFunction(U (*function)(T&, void*), void * arg){
      return (*function)(*mPtr, arg);
    }
    
  };
  
  T* mInstance;
  Mutex mMutex;
  
  //AtomicWrapper(const AtomicWrapper&&){}
  AtomicWrapper& operator=(const AtomicWrapper&){}
  AtomicWrapper(const AtomicWrapper&){}
  
public:
  
  AtomicWrapper(T* instance):mInstance(instance){}
  AtomicWrapper():mInstance(NULL){}
  
  void initialize(T* instance){
    mMutex.lock();
    mInstance = instance;
    mMutex.unlock();
  }

  const PtrWrapper operator->(){
    if(NULL == mInstance){SYS_ERROR("Operacja na niezainicjalizowanym AtomicWrapperze."); exit(EXIT_CODE_COUNTER);}
    return PtrWrapper(mInstance, mMutex);
  }

  T operator*(){ // zwraca kopie trzymanego obiektu
    if(NULL == mInstance){SYS_ERROR("Operacja na niezainicjalizowanym AtomicWrapperze."); exit(EXIT_CODE_COUNTER);}
    return *PtrWrapper(mInstance, mMutex);
  }
  
  template <typename U>
  U runFunction(U (*function)(T&, void*), void * arg = NULL){
    if(NULL == mInstance){SYS_ERROR("Operacja na niezainicjalizowanym AtomicWrapperze."); exit(EXIT_CODE_COUNTER);}
    return PtrWrapper(mInstance, mMutex).runFunction(function, arg);
  }
  
};

/////

template<typename T>
class GetSetWrapper{
  T val;
  
public:
  
  GetSetWrapper(const T & _val):val(_val){}
  GetSetWrapper(){}
  
  T get(){
    return val;
  }
  
  T set(T newVal){
    val = newVal;
  }
  
};

/////

pair<int, int> operator+(const pair<int, int> & P1, const pair<int, int> & P2){
  
  return make_pair(
                    P1.first  + P2.first,
                    P1.second + P2.second
                  );
  
}

/////

void interrupt(int signo){ exit(0); }

const string bialeZnaki = " \t\r";

const int WIELKOSC_BACKLOGU = 100;

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
  
  sendString(handler, msg, newline);
}

string recieveString(FILE * handler){
  string ret;
  do ret += getc(handler); while('\n' != ret[ret.size()-1]);
  return ret.erase(ret.size()-1);
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

/////

class Pole{
  
  // wszystkie wartosci dotycza stanu z poczatku tury
  
  bool wyspa; // czy na tym polu znajduje sie wyspa
  int  B,     // sumaryczna ilosc zuczkow na tym polu
       G,     // ilosc dzialajacych straznikow na tym polu
       R_O,   // liczba tratw z kapitanem
       R_A,   // sumaryczna liczba porzuconych na tym polu tratw
       S_O,   // sumaryczna liczba patykow na tratwach z kapitanem
       S_A;   // sumaryczna liczba patykow na porzuconych tratwach
  
public:
  
  Pole(bool _wyspa = false,
       int _B      = 0,
       int _G      = 0,
       int _R_O    = 0,
       int _R_A    = 0,
       int _S_O    = 0,
       int _S_A    = 0):
       wyspa(_wyspa), B(_B), G(_G), R_O(_R_O), R_A(_R_A), S_O(_S_O), S_A(_S_A){}
  
  void setWyspa(bool _wyspa){ wyspa = _wyspa; }
  void setB    (int  _B)    { B     = _B;     }
  void setG    (int  _G)    { G     = _G;     }
  void setR_O  (int  _R_O)  { R_O   = _R_O;   }
  void setR_A  (int  _R_A)  { R_A   = _R_A;   }
  void setS_O  (int  _S_O)  { S_O   = _S_O;   }
  void setS_A  (int  _S_A)  { S_A   = _S_A;   }
  
  bool getWyspa(){ return wyspa; }
  int  getB    (){ return B;     }
  int  getG    (){ return G;     }
  int  getR_O  (){ return R_O;   }
  int  getR_A  (){ return R_A;   }
  int  getS_O  (){ return S_O;   }
  int  getS_A  (){ return S_A;   }
  
  string getWyspaAsString(){
    if(wyspa) return "LAND";
    return "WATER";
  }
  
  void incrementB(){ ++B; }
  void decrementB(){
    if(0 == B){SYS_ERROR("Dekrementacja ilosci zuczkow na polu, gdzie nie ma zuczkow."); exit(EXIT_CODE_COUNTER);}
    --B;
  }
  
};

class Wyspa{
  
  int              Sticks;     // sumaryczna liczba patykow na wyspie
  map<string, int> teamSticks; // zbior liczb patykow przypisanych danej druzynie, uszeregowanych wedlug nazw druzyn
  
public:
  
  Wyspa(int _Sticks):Sticks(_Sticks){}
  
  int getSticks(){ return Sticks; }
  
  int getTeamsSticks(string nazwaDruzyny){
    map<string, int>::iterator it = teamSticks.find(nazwaDruzyny);
    if(teamSticks.end() == it) return 0;
    return it->second;
  }
  
  int takeSticks(int howMany, string nazwaDruzyny){
NYI //TODO
  }
  
  void leaveSticks(int howMany, string nazwaDruzyny){
NYI //TODO
  }
  
};

class MyWood{
  
  int T; // suma punktow przyznana druzynie za patyki lezace na wyspach (bez uwzgledniania wspolczynnika K)
  int S; // liczba patykow oznakowanych przez druzyne i lezacych na wyspach
  int C; // suma liczby patykow transportowanych przez druzyne (samodzielnie przez rozbitkow i na tratwach)
  
public:
  
  MyWood(int _T = 0, int _S = 0, int _C = 0):
    T(_T),
    S(_S),
    C(_C){}
  
  int getT(){ return T; }
  int getS(){ return S; }
  int getC(){ return C; }
  
  void addToT(int val){ T += val; }
  void addToS(int val){ S += val; }
  void addToC(int val){ C += val; }
  
};

class Zuczek{
  
  enum RoleEnum{
    GUARD,
    CAPTAIN,
    BUILDER,
    NONE
  };
  
  pair<int, int> zuczekCoords;
  int carriedSticks;
  int BusyCounter; // liczba tur, przez ktore zukoskoczek bedzie niedostepny wykonujac wczesniejsze rozkazy lub UNKNOWN dla straznikow
  RoleEnum Role;
  bool utopiony;
  
public:
  
  const static int UNKNOWN = -1;
  
  Zuczek(const pair<int, int> & _zuczekCoords, int _carriedSticks = 0, int _BusyCounter = 0, RoleEnum _Role = NONE, bool _utopiony = false):
    zuczekCoords(_zuczekCoords),
    carriedSticks(_carriedSticks),
    BusyCounter(_BusyCounter),
    Role(_Role),
    utopiony(_utopiony){}
  
  pair<int, int> getZuczekCoords() { return zuczekCoords;  }
  int            getCarriedSticks(){ return carriedSticks; }
  int            getBusyCounter()  { return BusyCounter;   }
  RoleEnum       getRole()         { return Role;          }
  bool           getUtopiony()     { return utopiony;      }
  
  string getBusyAsString(){
    if(UNKNOWN == BusyCounter)
      return "UNKNOWN";
    
    return NumberToString(BusyCounter);
  }
  string getRoleAsString(){
    
    switch(Role){
      case GUARD:   return "GUARD";
      case CAPTAIN: return "CAPTAIN";
      case BUILDER: return "BUILDER";
      case NONE:    return "NONE";
    }
    
    SYS_ERROR("Jezeli program doszedl do tego miejsca, cos jest nie w porzadku."); exit(EXIT_CODE_COUNTER);
    
  }
  
  void setCarriedSticks (int _carriedSticks){ carriedSticks = _carriedSticks; }
  void setRole          (RoleEnum _Role)    { Role = _Role;                   }
  
  void move(int dX, int dY){
    if(1 != abs(dX) &&
       1 != abs(dY) &&
       1 != abs(dX+dY)
    ) {SYS_ERROR("Proba przesuniecia zuczka o inna liczbe pol niz 1."); exit(EXIT_CODE_COUNTER);}
    
    zuczekCoords.first  += dX;
    zuczekCoords.second += dY;
  }
  
  void decrementBusyCounterIfBusy(){
    if(BusyCounter > 0)
      --BusyCounter;
  }
  
  void utopZuczka(){
    utopiony = true;
  }
  
};

class DescribeWorld{
  
  int N,    // dlugosc boku planszy
      I,    // liczba wysp na planszy
      Smin, // minimalny rozmiar ogniska
      F,    // mnoznik punktow za patyki w ognisku
      T;    // czas trwania pojedynczej tury w sekundach

  double K; // wspolczynnik skalujacy wynik
  
public:
  
  DescribeWorld(int _N, int _I, int _Smin, int _F, int _T, double _K):
    N(_N), I(_I), Smin(_Smin), F(_F), T(_T), K(_K){}
  
  void set(const DescribeWorld & values){
    *this = values;
  }
  
//   DescribeWorld getCopy(){
//     return *this;
//   }
  
  void setN   (int    _N)   { N    = _N;    }
  void setI   (int    _I)   { I    = _I;    }
  void setSmin(int    _Smin){ Smin = _Smin; }
  void setF   (int    _F)   { F    = _F;    }
  void setT   (int    _T)   { T    = _T;    }
  void setK   (double _K)   { K    = _K;    }
  
  int    getN   (){ return N;    }
  int    getI   (){ return I;    }
  int    getSmin(){ return Smin; }
  int    getF   (){ return F;    }
  int    getT   (){ return T;    }
  double getK   (){ return K;    }
  
};

class Top5_Element{
  pair<int, int> coords;
  int Sticks;
  
public:
  
  Top5_Element(const pair<int, int> & _coords, int _Sticks):
    coords(_coords),
    Sticks(_Sticks){}
  
  int getSticks()            const { return Sticks; }
  pair<int, int> getCoords() const { return coords; }
  
  bool operator>(const Top5_Element & A) const{
    if(Sticks == A.Sticks){
      if(coords.first == A.coords.first)
        return coords.second > A.coords.second;
      
      return coords.first > A.coords.first;
    }
    return Sticks > A.Sticks;
  }
  
};

struct ListWoodResult{
  
  pair<int, int> zuczekCoords;
  vector<pair<pair<int,int>, Wyspa> > wspolrzedneIDaneWysp;
  
  ListWoodResult(const pair<int, int> & _zuczekCoords):
    zuczekCoords(_zuczekCoords){}
  
};

/////
// wartosci opisujace stan gry

AtomicWrapper<DescribeWorld>                              ParametryRozgrywki;    // parametry rozgrywki i wartosc wspolczynnika skalujacego wynik


AtomicWrapper<GetSetWrapper<int> >                        PoczatkoweB,           // ilosc zukoskoczkow na poczatku rundy
                                                          L;                     // liczba tur do konca rundy

AtomicWrapper<GetSetWrapper<time_t> >                     Tstart;                // czas, kiedy zaczela sie tura

AtomicWrapper<GetSetWrapper<bool> >                       FireStatus;            // czy plonie ognisko

AtomicWrapper<vector<vector<Pole> > >                     Mapa;                  // "wektor dwuwymiarowy" przechowujacy podstawowe informacje o wszystkich polach na mapie gry
AtomicWrapper<map<pair<int, int>, Wyspa> >                Wyspy;                 // zbior informacji o wyspach, uszeregowanych wedlug ich wspolrzednych
AtomicWrapper<vector<pair<int, int> > >                   WyspyKeys;             // wektor zawierajacy wszystkie klucze mapy przechowywanej przez Wyspy

AtomicWrapper<set<Top5_Element, greater<Top5_Element> > > Top5;                  // zbior pieciu wysp, na ktorych na poczatku tury znajdowalo sie najwiecej patykow

AtomicWrapper<map<string, int> >                          BPerDruzyna;           // liczba zywych zukoskoczkow danej druzyny
AtomicWrapper<map<string, set<int> > >                    RozbitkowiePerDruzyna; // zbior identyfikatorow zywych zukoskoczkow danej druzyny
AtomicWrapper<map<string, MyWood> >                       MyWoodPerDruzyna;      // informacje zwracane w odpowiedzi na komende MY_WOOD, przydatne rowniez przy obliczaniu rankingu, zwiazane z dana druzyna

AtomicWrapper<vector<Zuczek> >                            Zuczki;                // zbior zuczkow, indeks zuczka w tym wektorze to jego identyfikator

/////

void zerujMapeFn(vector<vector<Pole> > &, void *);
void losujWyspyFn(map<pair<int, int>, Wyspa> &, void *);
void generujTop5Fn(map<pair<int, int>, Wyspa> &, void *);
void getListWoodResult(map<pair<int, int>, Wyspa> &, void *);

/////

int sprawdzZuczka(int ID, string nazwaDruzyny){ // funkcja zwraca 0 jesli zuczek istnieje i nalezy do zadanej druzyny, w przeciwnym przypadku zwraca kod bledu
  
  int ZuczkiSizeDummy = Zuczki->size();
  set<int> rpdDummy = RozbitkowiePerDruzyna->find(nazwaDruzyny)->second;
  
  if(0 > ID || ID >= ZuczkiSizeDummy || rpdDummy.end() == rpdDummy.find(ID))
    return 101;
  
  Zuczek zuczekDummy = Zuczki->at(ID);
  
  if(zuczekDummy.getUtopiony())
    return 105;
  
  
  return 0;
}

/////

void * watekPerKlient(void* _arg){
  
  FILE * handlerSocketu = ((pair<FILE *, string> *)_arg)->first;
  string nazwaDruzyny = ((pair<FILE *, string> *)_arg)->second;
  
  vector<string> komenda;
  
  for(;;){
    
    // TODO limit komend na turę
    
    komenda = myExplode(recieveString(handlerSocketu));
    
    if("DESCRIBE_WORLD" == komenda[0]){
      
      if(komenda.size() < 1) sendError(handlerSocketu, 3);
      else if(komenda.size() > 1) sendError(handlerSocketu, 4);
      else{
        sendString(handlerSocketu, "OK");
        
        DescribeWorld dwDummy = *ParametryRozgrywki;
        sendString(handlerSocketu,
                   NumberToString(dwDummy.getN())    + " " +
                   NumberToString(dwDummy.getI())    + " " +
                   NumberToString(dwDummy.getSmin()) + " " +
                   NumberToString(dwDummy.getF())    + " " +
                   NumberToString(dwDummy.getT())    + " " +
                   NumberToString(dwDummy.getK())
                  );
      }
      
    }
    else if("TIME_TO_RESCUE" == komenda[0]){
      
      if(komenda.size() < 1) sendError(handlerSocketu, 3);
      else if(komenda.size() > 1) sendError(handlerSocketu, 4);
      else{
        sendString(handlerSocketu, "OK");
        
        sendString(handlerSocketu,
                   string(FireStatus->get() ? "BURNING" : "NONE") + " " +
                   NumberToString(L->get())
                  );
        
        set<Top5_Element, greater<Top5_Element> > t5Dummy = *Top5;
        
        if(5 != t5Dummy.size()) {SYS_ERROR("Niespojnosc danych - Top5 nie ma pieciu elementow."); exit(EXIT_CODE_COUNTER);}
        
        for(set<Top5_Element, greater<Top5_Element> >::iterator it = t5Dummy.begin();
            t5Dummy.end() != it;
            ++it)
          sendString(handlerSocketu,
                     NumberToString(it->getCoords().first+1)  + " " +
                     NumberToString(it->getCoords().second+1) + " " +
                     NumberToString(it->getSticks())
                    );
        
      }
      
    }
    else if("LIST_SURVIVORS" == komenda[0]){
      
      if(komenda.size() < 1) sendError(handlerSocketu, 3);
      else if(komenda.size() > 1) sendError(handlerSocketu, 4);
      else{
        sendString(handlerSocketu, "OK");
        
        sendString(handlerSocketu,
                   NumberToString(BPerDruzyna->find(nazwaDruzyny)->second)
                  );
        
        set<int> rpdDummy = RozbitkowiePerDruzyna->find(nazwaDruzyny)->second;
        
        string msg = "";
        
        for(set<int>::iterator it = rpdDummy.begin(); rpdDummy.end() != it; ++it)
          msg += NumberToString(*it) + " ";
        
        msg.erase(msg.size()-1);
        
        sendString(handlerSocketu,
                   msg
                  );
        
      }
      
    }
    else if("LIST_RAFTS" == komenda[0]){
      
NYI //TODO
      
    }
    else if("IGNITION" == komenda[0]){
      
NYI //TODO
      
    }
    else if("MOVE" == komenda[0]){
      
NYI //TODO
      
    }
    else if("TAKE" == komenda[0]){
      
NYI //TODO
      
    }
    else if("GIVE" == komenda[0]){
      
NYI //TODO
      
    }
    else if("GUARD" == komenda[0]){
      
NYI //TODO
      
    }
    else if("STOP_GUARDING" == komenda[0]){
      
NYI //TODO
      
    }
    else if("LIST_WOOD" == komenda[0]){
      
      if(komenda.size() < 2) sendError(handlerSocketu, 3);
      else if(komenda.size() > 2) sendError(handlerSocketu, 4);
      else{
        
        int kodBledu;
        
        char * endptr;
        int ID = strtol(komenda[1].c_str(), &endptr, 0);
        if(*endptr)
          sendError(handlerSocketu, 3);
        else if(kodBledu = sprawdzZuczka(ID, nazwaDruzyny))
          sendError(handlerSocketu, kodBledu);
        else{
          
          Zuczek zuczekDummy = Zuczki->at(ID);
          
          if(Mapa->at(zuczekDummy.getZuczekCoords().first).at(zuczekDummy.getZuczekCoords().second).getWyspa() == false)
            sendError(handlerSocketu, 104);
          else{
            
            sendString(handlerSocketu, "OK");
            
            ListWoodResult listWoodResult(zuczekDummy.getZuczekCoords());
            Wyspy.runFunction(getListWoodResult, (void*)&listWoodResult);
            
            sendString(handlerSocketu,
                       NumberToString(listWoodResult.wspolrzedneIDaneWysp.size())
                      );
            
            for(int i=0; i<listWoodResult.wspolrzedneIDaneWysp.size(); ++i){
              
              sendString(handlerSocketu,
                         NumberToString(listWoodResult.wspolrzedneIDaneWysp[i].first.first+1)      + " " +
                         NumberToString(listWoodResult.wspolrzedneIDaneWysp[i].first.second+1)     + " " +
                         NumberToString(listWoodResult.wspolrzedneIDaneWysp[i].second.getSticks()) + " " +
                         NumberToString(listWoodResult.wspolrzedneIDaneWysp[i].second.getTeamsSticks(nazwaDruzyny))
                        );
              
            }
            
          }
        }
      }
    }
    else if("INFO" == komenda[0]){
      
      if(komenda.size() < 2) sendError(handlerSocketu, 3);
      else if(komenda.size() > 2) sendError(handlerSocketu, 4);
      else{
        
        int kodBledu;
        
        char * endptr;
        int ID = strtol(komenda[1].c_str(), &endptr, 0);
        if(*endptr)
          sendError(handlerSocketu, 3);
        else if(kodBledu = sprawdzZuczka(ID, nazwaDruzyny))
          sendError(handlerSocketu, kodBledu);
        else{
          
          Zuczek zuczekDummy = Zuczki->at(ID);
          
          sendString(handlerSocketu, "OK");
          
          sendString(handlerSocketu,
                     NumberToString(zuczekDummy.getZuczekCoords().first+1)  + " " +
                     NumberToString(zuczekDummy.getZuczekCoords().second+1) + " " +
                     NumberToString(zuczekDummy.getCarriedSticks())         + " " +
                     NumberToString(zuczekDummy.getBusyAsString())          + " " +
                     NumberToString(zuczekDummy.getRoleAsString())
                    );
          
          const pair<int, int> Sasiedzi[] = {
            make_pair( 0, 0),
            make_pair( 1, 0),
            make_pair( 0, 1),
            make_pair(-1, 0),
            make_pair( 0,-1)
          };
          
          for(int i=0; i<sizeof(Sasiedzi)/sizeof(pair<int, int>); ++i){
            
            pair<int, int> fieldCoords = zuczekDummy.getZuczekCoords() + Sasiedzi[i];
            
            int N_Dummy = ParametryRozgrywki->getN();
            
            if(fieldCoords.first < 0        || fieldCoords.second < 0 ||
               fieldCoords.first >= N_Dummy || fieldCoords.second >= N_Dummy)
              sendString(handlerSocketu,
                     "NIL"
                    );
            else{
              
              Pole PoleDummy = Mapa->at(fieldCoords.first).at(fieldCoords.second);
              
              sendString(handlerSocketu,
                         PoleDummy.getWyspaAsString()       + " " +
                         NumberToString(Sasiedzi[i].first)  + " " +
                         NumberToString(Sasiedzi[i].second) + " " +
                         NumberToString(PoleDummy.getB())   + " " +
                         NumberToString(PoleDummy.getG())   + " " +
                         NumberToString(PoleDummy.getR_O()) + " " +
                         NumberToString(PoleDummy.getR_A()) + " " +
                         NumberToString(PoleDummy.getS_O()) + " " +
                         NumberToString(PoleDummy.getS_A())
                        );
              
            }
            
          }
          
        }
      }
    }
    else if("BUILD" == komenda[0]){
      
NYI //TODO
      
    }
    else if("ABANDON" == komenda[0]){
      
NYI //TODO
      
    }
    else if("TAKE_OVER" == komenda[0]){
      
NYI //TODO
      
    }
    else if("DRY" == komenda[0]){
      
NYI //TODO
      
    }
    else if("MY_WOOD" == komenda[0]){
      
      if(komenda.size() < 1) sendError(handlerSocketu, 3);
      else if(komenda.size() > 1) sendError(handlerSocketu, 4);
      else{
        sendString(handlerSocketu, "OK");
        
        MyWood mwDummy = MyWoodPerDruzyna->find(nazwaDruzyny)->second;
        sendString(handlerSocketu,
                   NumberToString(mwDummy.getT())    + " " +
                   NumberToString(mwDummy.getS())    + " " +
                   NumberToString(mwDummy.getC())
                  );
      }
      
    }
    else if("WAIT" == komenda[0]){
      
NYI //TODO
      
    }
    else
      sendError(handlerSocketu, 2);
    
  }
  
} // koniec watku per klient

map<string, string> daneDruzyn;

void * watekAkceptora(void*){
  
  for(;;){
    int deskryptorSocketuKlienta =  accept(deskryptorSocketuAkceptora, NULL, NULL);
    if(-1 == deskryptorSocketuKlienta){SYS_ERROR("accept error"); continue;}
    
    FILE * handlerSocketuKlienta = fdopen(deskryptorSocketuKlienta, "r+");
    if(NULL == handlerSocketuKlienta){SYS_ERROR("fdopen error"); continue;}
    
    sendString(handlerSocketuKlienta, "LOGIN");
    string nazwaDruzynyKlienta = recieveString(handlerSocketuKlienta);
    sendString(handlerSocketuKlienta, "PASS");
    string haslo = recieveString(handlerSocketuKlienta);
    
    map<string, string>::iterator it = daneDruzyn.find(nazwaDruzynyKlienta);
    if(daneDruzyn.end() == it){
      sendError(handlerSocketuKlienta, 1);
      if(EOF == fclose(handlerSocketuKlienta)){SYS_ERROR("fclose error"); continue;}
    }
    else{
      sendString(handlerSocketuKlienta, "OK");
      
      handlerySocketowINazwyZespolowPerKlient.push_back(make_pair(handlerSocketuKlienta, nazwaDruzynyKlienta));
      
      cout << "Polaczono z klientem druzyny \"" << nazwaDruzynyKlienta << '"' << endl;
      
      pthread_t dummy;
      if(pthread_create(&dummy, NULL, watekPerKlient, (void*)&handlerySocketowINazwyZespolowPerKlient.back())){SYS_ERROR("pthread_create error"); exit(EXIT_CODE_COUNTER);}
    }
  }
  
} // koniec watku akceptora

/////

int main(int argc, char ** argv){
  
  srand(time(NULL));
  
  if(2 > argc || argc > 8){SYS_ERROR("Uzycie:\n"
                          "./serwer numerPortu czasTrwaniaTury=5 dlugoscBokuPlanszy=100 liczbaWysp=500 minimalnyRozmiarOgniska=1000 mnoznikPunktowZaPatykiWOgnisku=10 maksymalnaIloscTurNaRunde=1000\n"
                          "\n"
                          "kolejne argumenty przyjmuja wartosci z nastepujacych przedzialow:\n"
                          "\n"
                          "numerPortu - [1024;65535]\n"
                          "czasTrwaniaTury - [1;5]\n"
                          "dlugoscBokuPlanszy - [100;500]\n"
                          "liczbaWysp - [500;min(8000,dlugoscBokuPlanszy**2/5)]\n"
                          "minimalnyRozmiarOgniska - [1000;2000]\n"
                          "mnoznikPunktowZaPatykiWOgnisku - [10;20]\n"
                          "maksymalnaIloscTurNaRunde - [1000;10000]\n"
                          "\n"); return EXIT_CODE_COUNTER;}
  
  char * endptr;
  int numerPortu = strtol(argv[1], &endptr, 0);
  if(*endptr || 1024 > numerPortu || numerPortu > 65535){SYS_ERROR("Numer portu musi byc liczba calkowita z przedzialu [1024;65535]."); return EXIT_CODE_COUNTER;}
  
  const time_t CzasStartuGry = time(NULL);
  
  /////
  
  int czasTrwaniaTury = 5;
  int dlugoscBokuPlanszy = 100;
  int liczbaWysp = 500;
  int minimalnyRozmiarOgniska = 1000;
  int mnoznikPunktowZaPatykiWOgnisku = 10;
  int maksymalnaIloscTurNaRunde = 1000;
  
  if(argc > 2){
    czasTrwaniaTury = strtol(argv[2], &endptr, 0);
    if(*endptr || 1 > czasTrwaniaTury || czasTrwaniaTury > 5){SYS_ERROR("Czas trwania tury musi byc liczba calkowita z przedzialu [1;5]."); return EXIT_CODE_COUNTER;}
  }
  
  if(argc > 3){
    dlugoscBokuPlanszy = strtol(argv[3], &endptr, 0);
    if(*endptr || 100 > dlugoscBokuPlanszy || dlugoscBokuPlanszy > 500){SYS_ERROR("Dlugosc boku planszy musi byc liczba calkowita z przedzialu [100;500]."); return EXIT_CODE_COUNTER;}
  }
  
  if(argc > 4){
    liczbaWysp = strtol(argv[4], &endptr, 0);
    if(*endptr || 500 > liczbaWysp || liczbaWysp > min(8000, dlugoscBokuPlanszy*dlugoscBokuPlanszy/5)){SYS_ERROR("Liczba wysp musi byc liczba calkowita z przedzialu [500;" + NumberToString(min(8000, dlugoscBokuPlanszy*dlugoscBokuPlanszy/5)) + "] (ogolniej [500;min(8000,dlugoscBokuPlanszy**2/5)])."); return EXIT_CODE_COUNTER;}
  }
  
  if(argc > 5){
    minimalnyRozmiarOgniska = strtol(argv[5], &endptr, 0);
    if(*endptr || 1000 > minimalnyRozmiarOgniska || minimalnyRozmiarOgniska > 2000){SYS_ERROR("Minimalny rozmiar ogniska musi byc liczba calkowita z przedzialu [1000;2000]."); return EXIT_CODE_COUNTER;}
  }
  
  if(argc > 6){
    mnoznikPunktowZaPatykiWOgnisku = strtol(argv[6], &endptr, 0);
    if(*endptr || 10 > mnoznikPunktowZaPatykiWOgnisku || mnoznikPunktowZaPatykiWOgnisku > 20){SYS_ERROR("Mnoznik punktow za patyki w ognisku musi byc liczba calkowita z przedzialu [10;20]."); return EXIT_CODE_COUNTER;}
  }
  
  if(argc > 7){
    maksymalnaIloscTurNaRunde = strtol(argv[7], &endptr, 0);
    if(*endptr || 1000 > maksymalnaIloscTurNaRunde || maksymalnaIloscTurNaRunde > 10000){SYS_ERROR("Maksymalna ilosc tur na runde musi byc liczba calkowita z przedzialu [1000;10000]."); return EXIT_CODE_COUNTER;}
  }
  
  /////
  
  ifstream usersInputFileStream("users");
  if(usersInputFileStream.fail()){SYS_ERROR("Nie udalo sie otworzyc pliku \"users\", zawierajacego dane logowania druzyn."); return EXIT_CODE_COUNTER;}
  
  string login, pass;
  while(usersInputFileStream >> login >> pass)
    daneDruzyn[login] = pass;
  
  usersInputFileStream.clear();
  
  usersInputFileStream.close();
  if(usersInputFileStream.fail()){SYS_ERROR("ifstream::close error"); return EXIT_CODE_COUNTER;}
  
  /////

  if(atexit(clean)){SYS_ERROR("atexit error"); return EXIT_CODE_COUNTER;}
  if(SIG_ERR == signal(SIGINT, interrupt)){SYS_ERROR("signal error"); return EXIT_CODE_COUNTER;}
  
  /////
  
  deskryptorSocketuAkceptora = socket(AF_INET, SOCK_STREAM, 0);
  if(-1 == deskryptorSocketuAkceptora){SYS_ERROR("socket error"); return EXIT_CODE_COUNTER;}
  
  struct sockaddr_in adresSocketuAkceptora = {0};
  adresSocketuAkceptora.sin_family = AF_INET;
  adresSocketuAkceptora.sin_port = htons(numerPortu);
  adresSocketuAkceptora.sin_addr.s_addr = INADDR_ANY;

  if(-1 == bind(deskryptorSocketuAkceptora, (struct sockaddr *) &adresSocketuAkceptora, sizeof(struct sockaddr_in))){SYS_ERROR("bind error"); return EXIT_CODE_COUNTER;}
  
  if(-1 == listen(deskryptorSocketuAkceptora, WIELKOSC_BACKLOGU)){SYS_ERROR("listen error"); return EXIT_CODE_COUNTER;}
  
  {
    pthread_t dummy;
    if(pthread_create(&dummy, NULL, watekAkceptora, NULL)){SYS_ERROR("pthread_create error"); exit(EXIT_CODE_COUNTER);}
  }
  
  /////
  
  DescribeWorld                             nonatomic_ParametryRozgrywki(dlugoscBokuPlanszy, liczbaWysp, minimalnyRozmiarOgniska, mnoznikPunktowZaPatykiWOgnisku, czasTrwaniaTury, 1.);
  GetSetWrapper<int>                        nonatomic_PoczatkoweB;
  GetSetWrapper<int>                        nonatomic_L;
  GetSetWrapper<time_t>                     nonatomic_Tstart;
  GetSetWrapper<bool>                       nonatomic_FireStatus;
  vector<vector<Pole> >                     nonatomic_Mapa;
  map<pair<int, int>, Wyspa>                nonatomic_Wyspy;
  vector<pair<int, int> >                   nonatomic_WyspyKeys;
  set<Top5_Element, greater<Top5_Element> > nonatomic_Top5;
  map<string, int>                          nonatomic_BPerDruzyna;
  map<string, set<int> >                    nonatomic_RozbitkowiePerDruzyna;
  map<string, MyWood>                       nonatomic_MyWoodPerDruzyna;
  vector<Zuczek>                            nonatomic_Zuczki;
  
  
  ParametryRozgrywki   .initialize(&nonatomic_ParametryRozgrywki);
  PoczatkoweB          .initialize(&nonatomic_PoczatkoweB);
  L                    .initialize(&nonatomic_L);
  Tstart               .initialize(&nonatomic_Tstart);
  FireStatus           .initialize(&nonatomic_FireStatus);
  Mapa                 .initialize(&nonatomic_Mapa);
  Wyspy                .initialize(&nonatomic_Wyspy);
  WyspyKeys            .initialize(&nonatomic_WyspyKeys);
  Top5                 .initialize(&nonatomic_Top5);
  BPerDruzyna          .initialize(&nonatomic_BPerDruzyna);
  RozbitkowiePerDruzyna.initialize(&nonatomic_RozbitkowiePerDruzyna);
  MyWoodPerDruzyna     .initialize(&nonatomic_MyWoodPerDruzyna);
  Zuczki               .initialize(&nonatomic_Zuczki);
  
  /////
  
  const double Pod = 1.00002406790006336880498073623209268063; // podstawa funkcji wykladniczej wspolczynnika skalujacego wynik od czasu
  
  for(;;){ // petla po rundach
    
    int dummy_PoczatkoweB = rand()%8+3;
    
    ParametryRozgrywki ->setK(pow(Pod, time(NULL)-CzasStartuGry));
    PoczatkoweB        ->set(dummy_PoczatkoweB);
    L                  ->set(maksymalnaIloscTurNaRunde);
    Tstart             ->set(time(NULL));
    FireStatus         ->set(false);
    
    Mapa.runFunction(zerujMapeFn);
    
    Wyspy.runFunction(losujWyspyFn);
    
    for(map<string, string>::iterator it = daneDruzyn.begin();
        daneDruzyn.end() != it;
        ++it
    ){
      BPerDruzyna->insert(make_pair(it->first, dummy_PoczatkoweB));
    }
    
// // niebezpieczny kod do debugu!!!
//     
//     for(int y=0;y<nonatomic_ParametryRozgrywki.getN();++y){
//       for(int x=0;x<nonatomic_ParametryRozgrywki.getN();++x){
//         if(nonatomic_Mapa[x][y].getWyspa()){
//           cout << "#(" << nonatomic_Wyspy.find(make_pair(x,y))->second.getSticks() << ')';
//           x += NumberToString(nonatomic_Wyspy.find(make_pair(x,y))->second.getSticks()).size()+2;
//         }
//         else
//           cout << '.';
//       }
//       cout << endl;
//     }
//     
//     cout << endl;
//     
//     for(set<Top5_Element, greater<Top5_Element> >::iterator it = nonatomic_Top5.begin(); nonatomic_Top5.end() != it; ++it)
//       cout << it->getSticks() << " (" << it->getCoords().first << ',' << it->getCoords().second << ')' << endl;
//     
// // koniec niebezpiecznego kodu
    
    RozbitkowiePerDruzyna->clear();
    MyWoodPerDruzyna     ->clear();
    
    for(map<string, string>::iterator daneIt = daneDruzyn.begin();
        daneDruzyn.end() != daneIt;
        ++daneIt
    ){
      RozbitkowiePerDruzyna->insert(make_pair(daneIt->first, set<int>() ));
      MyWoodPerDruzyna     ->insert(make_pair(daneIt->first, MyWood()   ));
    }
    
    
    
    Zuczki->clear();
    
    {
      map<string, string>::iterator daneIt = daneDruzyn.begin();
      for(int idZuczka = 0; idZuczka < daneDruzyn.size()*dummy_PoczatkoweB; ++idZuczka){
        
        string nazwaDruzynyZuczka = daneIt->first;
        
        int indeksWyspyZuczka = rand() % WyspyKeys->size();
        pair<int, int> zuczekCoords = WyspyKeys->at(indeksWyspyZuczka);
        
        Mapa->at(zuczekCoords.first).at(zuczekCoords.second).incrementB();
        
        Zuczki->push_back(Zuczek(zuczekCoords));
        
        RozbitkowiePerDruzyna->find(nazwaDruzynyZuczka)->second.insert(idZuczka);
        
        if(0 == (idZuczka+1)%dummy_PoczatkoweB) ++daneIt;
      }
    }
    
    cout << "*** Nowa runda ***" << endl;
    
    for(;L->get();
        L->set((*L).get()-1), // kopiujemy L, odejmujemy 1 od kopii i przypisujemy
        Tstart->set(time(NULL))
       ){ // petla po turach
      
      Wyspy.runFunction(generujTop5Fn);
      
      int dummy_ZuczkiSize = Zuczki->size();
      
      for(int i=0; i<dummy_ZuczkiSize; ++i){
        Zuczki->at(i).decrementBusyCounterIfBusy();
        pair<int, int> zuczekCoords = Zuczki->at(i).getZuczekCoords();
        if(Mapa->at(zuczekCoords.first).at(zuczekCoords.second).getWyspa() == false)
          if(rand()%1000 == 0)
            Zuczki->at(i).utopZuczka();
      }
      
      time_t T_dummy = ParametryRozgrywki->getT();
      mySleep(T_dummy - 0.1);
      
      cout << "Nowa tura." << endl;
    }
    
  }

}

/////

bool czyMoznaPostawicWyspe(const map<pair<int, int>, Wyspa> & unwrapped_Wyspy, const pair<int, int> & newIslandsCoords){
  
  const static pair<int, int> Sasiedzi[] = {
    make_pair( 0, 0),
    make_pair( 1, 0),
    make_pair( 0, 1),
    make_pair(-1, 0),
    make_pair( 0,-1)
  };
  
  for(int i=0;i<sizeof(Sasiedzi)/sizeof(pair<int, int>);++i)
    if(unwrapped_Wyspy.end() != unwrapped_Wyspy.find(newIslandsCoords+Sasiedzi[i]) ) return false;
  
  return true;
}

/////

void zerujMapeFn(vector<vector<Pole> > & unwrapped_Mapa, void *){
  
  int dummy_N    = ParametryRozgrywki->getN();
  
  unwrapped_Mapa = vector<vector<Pole> >(
                                          dummy_N,
                                          vector<Pole>(dummy_N)
                                        );
  
}

void losujWyspyFn(map<pair<int, int>, Wyspa> & unwrapped_Wyspy, void *){
  
  unwrapped_Wyspy.clear();
  WyspyKeys->clear();
  
  int dummy_I    = ParametryRozgrywki->getI(),
      dummy_N    = ParametryRozgrywki->getN(),
      dummy_Smin = ParametryRozgrywki->getSmin();
  
  for(int i=0;i<dummy_I;++i){
    pair<int, int> newIslandsCoords;
    do newIslandsCoords = make_pair(rand()%dummy_N, rand()%dummy_N); while(!czyMoznaPostawicWyspe(unwrapped_Wyspy, newIslandsCoords));
    
    int iloscPatykowNaNowejWyspie = rand()%20 + (dummy_Smin+dummy_I-1)/dummy_I;
    unwrapped_Wyspy.insert(make_pair(newIslandsCoords, Wyspa(iloscPatykowNaNowejWyspie)));
    Mapa->at(newIslandsCoords.first).at(newIslandsCoords.second).setWyspa(true);
    WyspyKeys->push_back(newIslandsCoords);
  }
  
}

void generujTop5Fn(map<pair<int, int>, Wyspa> & unwrapped_Wyspy, void *){
  
  set<Top5_Element, greater<Top5_Element> > dummy_Top5;
  
  map<pair<int, int>, Wyspa>::iterator it = unwrapped_Wyspy.begin();
  
  for(int i=0;i<5;++i){
    dummy_Top5.insert(Top5_Element(it->first, it->second.getSticks()));
    ++it;
  }
  
  for(;unwrapped_Wyspy.end() != it; ++it){
    dummy_Top5.insert(Top5_Element(it->first, it->second.getSticks()));
    set<Top5_Element, greater<Top5_Element> >::iterator endIt = dummy_Top5.end();
    --endIt;
    dummy_Top5.erase(endIt);
  }
  
  Top5->swap(dummy_Top5);
  
}

void getListWoodResult(map<pair<int, int>, Wyspa> & unwrapped_Wyspy, void * _listWoodResult){
  
  ListWoodResult * listWoodResult = (ListWoodResult *) _listWoodResult;
  
  for(map<pair<int, int>, Wyspa>::iterator it = unwrapped_Wyspy.begin();
      unwrapped_Wyspy.end() != it;
      ++it){
    
    if(abs(it->first.first  - listWoodResult->zuczekCoords.first ) <= 8
    || abs(it->first.second - listWoodResult->zuczekCoords.second) <= 8
    ){
      listWoodResult->wspolrzedneIDaneWysp.push_back(*it);
    }
    
  }
  
}

















