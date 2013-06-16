#ifndef GAMESTATE_HPP
#define GAMESTATE_HPP



void zerujMapeFn(vector<vector<Pole> > &, void *);
void losujWyspyFn(map<pair<int, int>, Wyspa> &, void *);
void generujTop5Fn(map<pair<int, int>, Wyspa> &, void *);
void getListWoodResultFn(map<pair<int, int>, Wyspa> &, void *);
void zbieraniePatykowFn(map<pair<int, int>, Wyspa> &, void *);
void displayWorldFn(vector<vector<Pole> > &, void *);

/////
// wartosci opisujace stan gry

AtomicWrapper<DescribeWorld>                              ParametryRozgrywki;    // parametry rozgrywki i wartosc wspolczynnika skalujacego wynik


AtomicWrapper<GetSetWrapper<int> >                        PoczatkoweB,           // ilosc zukoskoczkow na poczatku rundy
                                                          L;                     // liczba tur do konca rundy

AtomicWrapper<GetSetWrapper<double> >                     Tstart;                // czas, kiedy zaczela sie tura

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

map<string, string> daneDruzyn;

#endif