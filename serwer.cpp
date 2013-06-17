#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <climits>
#include <cerrno>
#include <csignal>

#include <iostream>
#include <fstream>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>

using namespace std;

#include "header.hpp"
#include "wrappers.hpp"
#include "gamespecificclasses.hpp"
#include "gamestate.hpp"
#include "watekperklient.hpp"
#include "atomicwrapper_functions.hpp"

/////

void generujTop5Fn(map<pair<int, int>, Wyspa> &, void *);
void getListWoodResultFn(map<pair<int, int>, Wyspa> &, void *);
void zbieraniePatykowFn(map<pair<int, int>, Wyspa> &, void *);
void displayWorldFn(vector<vector<Pole> > &, void *);

/////

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

bool czyMoznaPostawicWyspe(const map<pair<int, int>, Wyspa> & nonatomic_Wyspy, const pair<int, int> & newIslandsCoords){
  
  const static pair<int, int> Sasiedzi[] = {
    make_pair( 0, 0),
    make_pair( 1, 0),
    make_pair( 0, 1),
    make_pair(-1, 0),
    make_pair( 0,-1)
  };
  
  for(int i=0;i<sizeof(Sasiedzi)/sizeof(pair<int, int>);++i)
    if(nonatomic_Wyspy.end() != nonatomic_Wyspy.find(newIslandsCoords+Sasiedzi[i]) ) return false;
  
  return true;
}

int main(int argc, char ** argv){
  
  srand(time(NULL));
  
  if(2 > argc || argc > 9){SYS_ERROR("Uzycie:\n"
                          "./serwer numerPortu czasTrwaniaTury=5 dlugoscBokuPlanszy=100 liczbaWysp=500 minimalnyRozmiarOgniska=1000 mnoznikPunktowZaPatykiWOgnisku=10 maksymalnaIloscTurNaRunde=1000 czasStartuGry=(aktualny czas)\n"
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
                          "czasStartuGry (czas uniksowy) - [0;" + NumberToString(INT_MAX) + "]\n"
                          "\n"); return EXIT_CODE_COUNTER;}
  
  char * endptr;
  int numerPortu = strtol(argv[1], &endptr, 0);
  if(*endptr || 1024 > numerPortu || numerPortu > 65535){SYS_ERROR("Numer portu musi byc liczba calkowita z przedzialu [1024;65535]."); return EXIT_CODE_COUNTER;}
  
  /////
  
  int czasTrwaniaTury = 5;
  int dlugoscBokuPlanszy = 100;
  int liczbaWysp = 500;
  int minimalnyRozmiarOgniska = 1000;
  int mnoznikPunktowZaPatykiWOgnisku = 10;
  int maksymalnaIloscTurNaRunde = 1000;
  double czasStartuGry = czasRzeczywisty();
  
//   NYI dlugoscBokuPlanszy = 30; liczbaWysp = 30; maksymalnaIloscTurNaRunde = 10; //TODO wyrzucic ta linie
  
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
  
  if(argc > 8){
    czasStartuGry = strtol(argv[8], &endptr, 0);
    if(*endptr || 0 > czasStartuGry || ERANGE == errno){SYS_ERROR("Czas startu gry musi byc liczba calkowita z przedzialu [0;" + NumberToString(INT_MAX) + "]."); return EXIT_CODE_COUNTER;}
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
  
  DescribeWorld                             nonatomic_ParametryRozgrywki(dlugoscBokuPlanszy, liczbaWysp, minimalnyRozmiarOgniska, mnoznikPunktowZaPatykiWOgnisku, czasTrwaniaTury, pow(Pod, czasRzeczywisty()-czasStartuGry));
  GetSetWrapper<int>                        nonatomic_PoczatkoweB       (rand()%8+3);
  GetSetWrapper<int>                        nonatomic_L                 (maksymalnaIloscTurNaRunde);
  GetSetWrapper<double>                     nonatomic_Tstart            (czasRzeczywisty());
  GetSetWrapper<bool>                       nonatomic_FireStatus        (false);
  vector<vector<Pole> >                     nonatomic_Mapa              (vector<vector<Pole> >(
                                                                            dlugoscBokuPlanszy,
                                                                            vector<Pole>(dlugoscBokuPlanszy)
                                                                          )
                                                                        );
  map<pair<int, int>, Wyspa>                nonatomic_Wyspy;
  vector<pair<int, int> >                   nonatomic_WyspyKeys;
  set<Top5_Element, greater<Top5_Element> > nonatomic_Top5;
  map<string, int>                          nonatomic_BPerDruzyna;
  map<string, set<int> >                    nonatomic_RozbitkowiePerDruzyna;
  map<string, MyWood>                       nonatomic_MyWoodPerDruzyna;
  vector<Zuczek>                            nonatomic_Zuczki;
  
  
  for(int i=0;i<liczbaWysp;++i){ // losuj wyspy
    pair<int, int> newIslandsCoords;
    do newIslandsCoords = make_pair(rand()%dlugoscBokuPlanszy, rand()%dlugoscBokuPlanszy); while(!czyMoznaPostawicWyspe(nonatomic_Wyspy, newIslandsCoords));
    
    int iloscPatykowNaNowejWyspie = rand()%20 + (minimalnyRozmiarOgniska+liczbaWysp-1)/liczbaWysp;
    
    nonatomic_Wyspy.insert(make_pair(newIslandsCoords, Wyspa(newIslandsCoords, iloscPatykowNaNowejWyspie)));
    
    nonatomic_Mapa[newIslandsCoords.first][newIslandsCoords.second].setWyspa(true);
    
    nonatomic_WyspyKeys.push_back(newIslandsCoords);
  }
  
  
  for(map<string, string>::iterator it = daneDruzyn.begin();
      daneDruzyn.end() != it;
      ++it){ // inicjalizuj liczbe zuczkow dla kazdej druzyny
    nonatomic_BPerDruzyna          [it->first] = nonatomic_PoczatkoweB.get();
    nonatomic_RozbitkowiePerDruzyna[it->first]; // wstawia pod tym kluczem set<int>(), czyli pusty zbior
    nonatomic_MyWoodPerDruzyna     [it->first]; // wstawia pod tym kluczem MyWood()
  }
  
  
  {
    map<string, string>::iterator it = daneDruzyn.begin();
    for(int idZuczka = 0; idZuczka < daneDruzyn.size()*nonatomic_PoczatkoweB.get(); ++idZuczka){
      
      string nazwaDruzynyZuczka = it->first;
      
      pair<int, int> zuczekCoords = nonatomic_WyspyKeys[rand() % nonatomic_WyspyKeys.size()];
      
      nonatomic_Mapa[zuczekCoords.first][zuczekCoords.second].incrementB();
      
      nonatomic_Zuczki.push_back(Zuczek(zuczekCoords, nazwaDruzynyZuczka));
      
      nonatomic_RozbitkowiePerDruzyna[nazwaDruzynyZuczka].insert(idZuczka);
      
      if(0 == (idZuczka+1)%nonatomic_PoczatkoweB.get()) ++it;
    }
  }
  
  
  
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
  
  for(;L->get();
      L->set((*L).get()-1), // kopiujemy L, odejmujemy 1 od kopii i przypisujemy
      Tstart->set(czasRzeczywisty())
     ){ // petla po turach
  
    ParametryRozgrywki ->setK(pow(Pod, czasRzeczywisty()-czasStartuGry));
    
    int dummy_ZuczkiSize = Zuczki->size();
    
    Wyspy.runFunction(zbieraniePatykowFn); // ta funkcja musi byc wywolana przed dekrementacja BusyCounterow
    
    for(int i=0; i<dummy_ZuczkiSize; ++i){
      Zuczki->at(i).makeMovement(Mapa);
      
      Zuczki->at(i).decrementBusyCounterIfBusy();
      
      pair<int, int> zuczekCoords = Zuczki->at(i).getZuczekCoords();
      if(Mapa->at(zuczekCoords.first).at(zuczekCoords.second).getWyspa() == false)
        if(rand()%1000 == 0)
          Zuczki->at(i).utopZuczka();
    }
  
    Wyspy.runFunction(generujTop5Fn);
    
    Mapa.runFunction(displayWorldFn);
    
    double czasOczekiwania = Tstart->get() + ParametryRozgrywki->getT() - czasRzeczywisty();
    mySleep(czasOczekiwania - 0.1);
    
//       cout << "Nowa tura." << endl;
  }
  
  
  //TODO wypisywanie punktacji
  
  return 0;
}