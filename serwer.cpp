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
  
};

class Wyspa{
  
  int              Sticks;     // sumaryczna liczba patykow na wyspie
  map<string, int> teamSticks; // zbior liczb patykow przypisanych danej druzynie, uszeregowanych wedlug nazw druzyn
  
public:
  
  Wyspa(int _Sticks):Sticks(_Sticks){}
  
  int getSticks(){ return Sticks; }
  
  int takeSticks(int howMany, string nazwaDruzyny){
NYI //TODO
  }
  
  void leaveSticks(int howMany, string nazwaDruzyny){
NYI //TODO
  }
  
};

class MyWood{
//TODO
};

class Zuczek{
//TODO
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

/////
// wartosci opisujace stan gry

AtomicWrapper<DescribeWorld>               ParametryRozgrywki;    // parametry rozgrywki i wartosc wspolczynnika skalujacego wynik


AtomicWrapper<GetSetWrapper<int> >         PoczatkoweB,           // ilosc zukoskoczkow na poczatku rundy
                                           L;                     // liczba tur do konca rundy

AtomicWrapper<GetSetWrapper<time_t> >      Tstart;                // czas, kiedy zaczela sie tura

AtomicWrapper<GetSetWrapper<bool> >        FireStatus;            // czy plonie ognisko

AtomicWrapper<vector<vector<Pole> > >      Mapa;                  // "wektor dwuwymiarowy" przechowujacy podstawowe informacje o wszystkich polach na mapie gry
AtomicWrapper<map<pair<int, int>, Wyspa> > Wyspy;                 // zbior informacji o wyspach, uszeregowanych wedlug ich wspolrzednych

AtomicWrapper<set<Wyspa> >                 Top5;                  // zbior pieciu wysp, na ktorych na poczatku tury znajdowalo sie najwiecej patykow

AtomicWrapper<map<string, int> >           BPerDruzyna;           // liczba zywych zukoskoczkow danej druzyny
AtomicWrapper<map<string, set<int> > >     RozbitkowiePerDruzyna; // zbior identyfikatorow zywych zukoskoczkow danej druzyny
AtomicWrapper<map<string, MyWood> >        MyWoodPerDruzyna;      // informacje zwracane w odpowiedzi na komende MY_WOOD, przydatne rowniez przy obliczaniu rankingu, zwiazane z dana druzyna

AtomicWrapper<map<int, Zuczek> >           Zuczki;                // zbior zuczkow, uszeregowanych wedlug ich ID

/////

void * watekPerKlient(void* _arg){
  
  FILE * handlerSocketu = ((pair<FILE *, string> *)_arg)->first;
  string nazwaDruzyny = ((pair<FILE *, string> *)_arg)->second;
  
  vector<string> komenda;
  
  for(;;){
    
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
      
NYI //TODO
      
    }
    else if("LIST_SURVIVORS" == komenda[0]){
      
NYI //TODO
      
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
      
NYI //TODO
      
    }
    else if("INFO" == komenda[0]){
      
NYI //TODO
      
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
      
NYI //TODO
      
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

void zerujMapeFn(vector<vector<Pole> > &, void *);
void losujWyspy(map<pair<int, int>, Wyspa> &, void *);

/////

int main(int argc, char ** argv){
  
  srand(time(NULL));
  
  if(2 > argc || argc > 7){SYS_ERROR("Uzycie:\n"
                          "./serwer numerPortu czasTrwaniaTury=5 dlugoscBokuPlanszy=100 liczbaWysp=500 minimalnyRozmiarOgniska=1000 mnoznikPunktowZaPatykiWOgnisku=10\n"
                          "\n"
                          "kolejne argumenty przyjmuja wartosci z nastepujacych przedzialow:\n"
                          "\n"
                          "numerPortu - [1024;65535]\n"
                          "czasTrwaniaTury - [1;5]\n"
                          "dlugoscBokuPlanszy - [100;500]\n"
                          "liczbaWysp - [500;min(8000,dlugoscBokuPlanszy**2/5)]\n"
                          "minimalnyRozmiarOgniska - [1000;2000]\n"
                          "mnoznikPunktowZaPatykiWOgnisku - [10;20]\n"
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
    if(*endptr || 500 > liczbaWysp || liczbaWysp > min(8000, dlugoscBokuPlanszy*dlugoscBokuPlanszy/5)){SYS_ERROR("Liczba wysp musi byc liczba calkowita z przedzialu [100;" + NumberToString(min(8000, dlugoscBokuPlanszy*dlugoscBokuPlanszy/5)) + "] (ogolniej [500;min(8000,dlugoscBokuPlanszy**2/5)])."); return EXIT_CODE_COUNTER;}
  }
  
  if(argc > 5){
    minimalnyRozmiarOgniska = strtol(argv[5], &endptr, 0);
    if(*endptr || 1000 > minimalnyRozmiarOgniska || minimalnyRozmiarOgniska > 2000){SYS_ERROR("Minimalny rozmiar ogniska musi byc liczba calkowita z przedzialu [1000;2000]."); return EXIT_CODE_COUNTER;}
  }
  
  if(argc > 6){
    mnoznikPunktowZaPatykiWOgnisku = strtol(argv[6], &endptr, 0);
    if(*endptr || 10 > mnoznikPunktowZaPatykiWOgnisku || mnoznikPunktowZaPatykiWOgnisku > 20){SYS_ERROR("Mnoznik punktow za patyki w ognisku musi byc liczba calkowita z przedzialu [10;20]."); return EXIT_CODE_COUNTER;}
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
  
  DescribeWorld              nonatomic_ParametryRozgrywki(dlugoscBokuPlanszy, liczbaWysp, minimalnyRozmiarOgniska, mnoznikPunktowZaPatykiWOgnisku, czasTrwaniaTury, 1.);
  GetSetWrapper<int>         nonatomic_PoczatkoweB;
  GetSetWrapper<int>         nonatomic_L;
  GetSetWrapper<time_t>      nonatomic_Tstart;
  GetSetWrapper<bool>        nonatomic_FireStatus;
  vector<vector<Pole> >      nonatomic_Mapa;
  map<pair<int, int>, Wyspa> nonatomic_Wyspy; //TODO
  set<Wyspa>                 nonatomic_Top5; //TODO
  map<string, int>           nonatomic_BPerDruzyna;
  map<string, set<int> >     nonatomic_RozbitkowiePerDruzyna; //TODO
  map<string, MyWood>        nonatomic_MyWoodPerDruzyna; //TODO
  map<int, Zuczek>           nonatomic_Zuczki; //TODO
  
  
  ParametryRozgrywki   .initialize(&nonatomic_ParametryRozgrywki);
  PoczatkoweB          .initialize(&nonatomic_PoczatkoweB);
  L                    .initialize(&nonatomic_L);
  Tstart               .initialize(&nonatomic_Tstart);
  FireStatus           .initialize(&nonatomic_FireStatus);
  Mapa                 .initialize(&nonatomic_Mapa);
  Wyspy                .initialize(&nonatomic_Wyspy);
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
    L                  ->set(1000);
    Tstart             ->set(time(NULL));
    FireStatus         ->set(false);
    
    Mapa.runFunction(zerujMapeFn);
    
    Wyspy.runFunction(losujWyspy);
    
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
// // koniec niebezpiecznego kodu
    
NYI return EXIT_CODE_COUNTER; //TODO losowanie pozycji startowych zuczkow
    
    for(;L->get();
        sleep(ParametryRozgrywki->getT()),
        L->set((*L).get()-1), // kopiujemy L, odejmujemy 1 od kopii i przypisujemy
        Tstart->set(time(NULL))
       ){ // petla po turach
      
NYI //TODO obsluga systemu turowego gry
      
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
  
  for(int i=0;i<sizeof(Sasiedzi);++i)
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

void losujWyspy(map<pair<int, int>, Wyspa> & unwrapped_Wyspy, void *){
  
  unwrapped_Wyspy.clear();
  
  int dummy_I    = ParametryRozgrywki->getI(),
      dummy_N    = ParametryRozgrywki->getN(),
      dummy_Smin = ParametryRozgrywki->getSmin();
  
  for(int i=0;i<dummy_I;++i){
    pair<int, int> newIslandsCoords;
    do newIslandsCoords = make_pair(rand()%dummy_N, rand()%dummy_N); while(!czyMoznaPostawicWyspe(unwrapped_Wyspy, newIslandsCoords));
    
    int iloscPatykowNaNowejWyspie = rand()%20 + (dummy_Smin+dummy_I-1)/dummy_I;
    unwrapped_Wyspy.insert(make_pair(newIslandsCoords, Wyspa(iloscPatykowNaNowejWyspie)));
    Mapa->at(newIslandsCoords.first).at(newIslandsCoords.second).setWyspa(true);
  }
  
}




















