#ifndef GAMESPECIFICCLASSES_HPP
#define GAMESPECIFICCLASSES_HPP



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
  string nazwaDruzyny;
  int carriedSticks;
  int BusyCounter; // liczba tur, przez ktore zukoskoczek bedzie niedostepny wykonujac wczesniejsze rozkazy lub UNKNOWN dla straznikow
  RoleEnum Role;
  bool utopiony;
  pair<int, int> enqueuedMovementRelativeCoords;
  
  void move(int dX, int dY){
    if( (1 != abs(dX) || 0 != abs(dY))
     && (0 != abs(dX) || 1 != abs(dY)) ) {SYS_ERROR("Proba przesuniecia zuczka o inna liczbe pol niz 1."); exit(EXIT_CODE_COUNTER);}
    
    zuczekCoords.first  += dX;
    zuczekCoords.second += dY;
  }
  
public:
  
  const static int UNKNOWN = -1;
  
  Zuczek(const pair<int, int> & _zuczekCoords, const string & _nazwaDruzyny, int _carriedSticks = 0, int _BusyCounter = 0, RoleEnum _Role = NONE, bool _utopiony = false):
    zuczekCoords(_zuczekCoords),
    nazwaDruzyny(_nazwaDruzyny),
    carriedSticks(_carriedSticks),
    BusyCounter(_BusyCounter),
    Role(_Role),
    utopiony(_utopiony),
    enqueuedMovementRelativeCoords(make_pair(0,0)){}
  
  pair<int, int> getZuczekCoords() { return zuczekCoords;  }
  string         getNazwaDruzyny() { return nazwaDruzyny;  }
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
  
//   void setCarriedSticks (int _carriedSticks){ carriedSticks = _carriedSticks; }
  void setBusyCounter   (int _BusyCounter)  { BusyCounter = _BusyCounter;     }
  void setRole          (RoleEnum _Role)    { Role = _Role;                   }
  
  void decrementBusyCounterIfBusy(){
    if(BusyCounter > 0)
      --BusyCounter;
  }
  
  void utopZuczka(){
    utopiony = true;
  }
  
  int getCapacity(){
    if(CAPTAIN == Role)
      return 40-carriedSticks;
    
    return 5-carriedSticks;
  }
  
  void addSticks(int numberOfSticks, AtomicWrapper<map<string, MyWood> > & MyWoodPerDruzyna){
    
    if(getCapacity() - numberOfSticks < 0) {SYS_ERROR("Proba dodania patykow do pelnego zuczka."); exit(EXIT_CODE_COUNTER);}
    
    MyWoodPerDruzyna->find(nazwaDruzyny)->second.addToC(numberOfSticks);
    carriedSticks += numberOfSticks;
  }
  
  bool makeBusy(int val){
    if(0 != BusyCounter) return false;
    
    BusyCounter = val;
    
    return true;
  }
  
  void enqueueMovement(pair<int, int> relativeCoords){
    enqueuedMovementRelativeCoords = relativeCoords;
  }
  
  void makeMovement(){
    
    if(make_pair(0,0) != enqueuedMovementRelativeCoords){
      move(enqueuedMovementRelativeCoords.first, enqueuedMovementRelativeCoords.second);
      enqueuedMovementRelativeCoords = make_pair(0,0);
    }
    
  }
  
  int giveAllCarriedSticks(AtomicWrapper<map<string, MyWood> > & MyWoodPerDruzyna){
    
    MyWoodPerDruzyna->find(nazwaDruzyny)->second.addToC( - carriedSticks );
    
    int ret = carriedSticks;
    carriedSticks = 0;
    return ret;
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

class Wyspa{
  
  pair<int, int>   coords;
  int              Sticks;        // sumaryczna liczba patykow na wyspie
  map<string, int> teamSticks;    // zbior liczb patykow przypisanych danej druzynie, uszeregowanych wedlug nazw druzyn
  list<int>        takersQueue;   // kolejka identyfikatorow zuczkow ktore chca zabrac patyki pod koniec tury
  list<int>        buildersQueue; // kolejka identyfikatorow zuczkow ktore chca zabrac patyki pod koniec tury pod budowe tratwy
  bool             burning;
  
public:
  
  Wyspa(const pair<int, int> & _coords, int _Sticks):
    coords(_coords),
    Sticks(_Sticks),
    burning(false){}
  
  void ignition(AtomicWrapper<map<string, MyWood> > & MyWoodPerDruzyna,
                AtomicWrapper<DescribeWorld> & ParametryRozgrywki){
    burning = true;
    takersQueue.clear();
    buildersQueue.clear();
    
    for(map<string, int>::iterator it = teamSticks.begin();
            teamSticks.end() != it;
            ++it){
          MyWoodPerDruzyna->find(it->first)->second.addToT( it->second * (ParametryRozgrywki->getF() - 1) );
        }
  }
  
  int getSticks(){ return Sticks; }
  
  int getTeamsSticks(string nazwaDruzyny){
    map<string, int>::iterator it = teamSticks.find(nazwaDruzyny);
    if(teamSticks.end() == it) return 0;
    return it->second;
  }
  
  void leaveSticks(int howMany, string nazwaDruzyny,
                   AtomicWrapper<map<string, MyWood> > & MyWoodPerDruzyna,
                   AtomicWrapper<DescribeWorld> & ParametryRozgrywki){
    
    Sticks += howMany;
    teamSticks[nazwaDruzyny] += howMany;
    MyWoodPerDruzyna->find(nazwaDruzyny)->second.addToS(howMany);
    if(burning)
      MyWoodPerDruzyna->find(nazwaDruzyny)->second.addToT(howMany * ParametryRozgrywki->getF());
    else
      MyWoodPerDruzyna->find(nazwaDruzyny)->second.addToT(howMany);
    
  }
  
  void enqueueTaker(int ID){ takersQueue.push_back(ID); }
  
  void letTakersTake(AtomicWrapper<vector<vector<Pole> > > & Mapa,
                     AtomicWrapper<vector<Zuczek> > & Zuczki,
                     AtomicWrapper<map<string, MyWood> > & MyWoodPerDruzyna){ // Ta funkcja jest thread-safe, dopoki zuczki do ktorych sie odwoluje maja ustawiony niezerowy BusyCounter
    
    if(Mapa->at(coords.first).at(coords.second).getG() >= takersQueue.size()+buildersQueue.size()){ // wycisk
      
      while(takersQueue.size()){
        Zuczki->at(takersQueue.front()).setBusyCounter(15);
        takersQueue.pop_front();
      }
      
    }
    else{ // samowola
      
      int zapotrzebowanie = buildersQueue.size()*100;
      
      for(list<int>::iterator it = takersQueue.begin(); takersQueue.end() != it; ++it)
        zapotrzebowanie += Zuczki->at(*it).getCapacity();
      
      if(zapotrzebowanie >= Sticks){
        
        for(map<string, int>::iterator it = teamSticks.begin();
            teamSticks.end() != it;
            ++it){
          MyWoodPerDruzyna->find(it->first)->second.addToS( - it->second );
          MyWoodPerDruzyna->find(it->first)->second.addToT( - it->second );
        }
        
        teamSticks.clear();
        
        // TODO przyznawanie patykow w pierwszej kolejnosci budowniczym
        
        double wspolczynnikZaspokojeniaZapotrzebowania = double(Sticks)/zapotrzebowanie;
        
        for(list<int>::iterator it = takersQueue.begin(); takersQueue.end() != it; ++it){
          int iloscWzietychPatykow = Zuczki->at(*it).getCapacity() * wspolczynnikZaspokojeniaZapotrzebowania;
          Sticks -= iloscWzietychPatykow;
          Zuczki->at(*it).addSticks(iloscWzietychPatykow, MyWoodPerDruzyna);
        }
        
        while(takersQueue.size() && Sticks > 0){
          
          int IDzbieracza = takersQueue.front();
          takersQueue.pop_front();
          
          int zapotrzebowanieZbieracza = Zuczki->at(IDzbieracza).getCapacity();
          
          if(zapotrzebowanieZbieracza > 0){
            --Sticks;
            Zuczki->at(IDzbieracza).addSticks(1, MyWoodPerDruzyna);
          }
          
        }
        
        takersQueue.clear();
        
      }
      else{
        
        // TODO obsluga budowniczych
        
        set<string> druzynyZbieraczy;
        
        for(list<int>::iterator it = takersQueue.begin(); takersQueue.end() != it; ++it)
          druzynyZbieraczy.insert(Zuczki->at(*it).getNazwaDruzyny());
        
        int liczbaPatykowDruzynZbieraczy = 0;
        int liczbaPatykowInnychDruzyn = 0;
        
        for(map<string, int>::iterator it = teamSticks.begin();
            teamSticks.end() != it;
            ++it){
          
          if(druzynyZbieraczy.end() == druzynyZbieraczy.find(it->first))
            liczbaPatykowInnychDruzyn += it->second;
          else
            liczbaPatykowDruzynZbieraczy += it->second;
          
        }
        
        int liczbaNiczyichPatykow = Sticks - liczbaPatykowDruzynZbieraczy - liczbaPatykowInnychDruzyn;
        
        
        while(takersQueue.size()){
          
          int IDzbieracza = takersQueue.front();
          takersQueue.pop_front();
          
          int zapotrzebowanieZbieracza = Zuczki->at(IDzbieracza).getCapacity();
          
          Zuczki->at(IDzbieracza).addSticks(zapotrzebowanieZbieracza, MyWoodPerDruzyna);
          
        }
        
        
        Sticks -= zapotrzebowanie;
        
        if(liczbaNiczyichPatykow >= zapotrzebowanie)
          liczbaNiczyichPatykow -= zapotrzebowanie;
        else{
          zapotrzebowanie -= liczbaNiczyichPatykow;
          liczbaNiczyichPatykow = 0;
          
          if(liczbaPatykowInnychDruzyn >= zapotrzebowanie){
            double wspolczynnikUdzialuInnychDruzyn = double(zapotrzebowanie)/liczbaPatykowInnychDruzyn;
            
            liczbaPatykowInnychDruzyn -= zapotrzebowanie;
            
            for(map<string, int>::iterator it = teamSticks.begin();
            teamSticks.end() != it;
            ++it)
              if(druzynyZbieraczy.end() == druzynyZbieraczy.find(it->first)){
                zapotrzebowanie -= it->second * wspolczynnikUdzialuInnychDruzyn;
                MyWoodPerDruzyna->find(it->first)->second.addToS( - it->second * wspolczynnikUdzialuInnychDruzyn );
                MyWoodPerDruzyna->find(it->first)->second.addToT( - it->second * wspolczynnikUdzialuInnychDruzyn );
                it->second -= it->second * wspolczynnikUdzialuInnychDruzyn;
              }
            
            for(map<string, int>::iterator it = teamSticks.begin();
            teamSticks.end() != it && zapotrzebowanie > 0;
            ++it)
              if(druzynyZbieraczy.end() == druzynyZbieraczy.find(it->first) && it->second > 0){
                --zapotrzebowanie;
                --it->second;
                MyWoodPerDruzyna->find(it->first)->second.addToS( -1 );
                MyWoodPerDruzyna->find(it->first)->second.addToT( -1 );
              }
          }
          else{
            zapotrzebowanie -= liczbaPatykowInnychDruzyn;
            liczbaPatykowInnychDruzyn = 0;
            
            for(map<string, int>::iterator it = teamSticks.begin();
            teamSticks.end() != it;
            ++it)
              if(druzynyZbieraczy.end() == druzynyZbieraczy.find(it->first)){
                MyWoodPerDruzyna->find(it->first)->second.addToS( - it->second );
                MyWoodPerDruzyna->find(it->first)->second.addToT( - it->second );
                it->second = 0;
              }
            
            
            double wspolczynnikUdzialuDruzyn = double(zapotrzebowanie)/liczbaPatykowDruzynZbieraczy;
            
            liczbaPatykowDruzynZbieraczy -= zapotrzebowanie;
            
            for(map<string, int>::iterator it = teamSticks.begin();
            teamSticks.end() != it;
            ++it)
              if(druzynyZbieraczy.end() != druzynyZbieraczy.find(it->first)){
                zapotrzebowanie -= it->second * wspolczynnikUdzialuDruzyn;
                MyWoodPerDruzyna->find(it->first)->second.addToS( - it->second * wspolczynnikUdzialuDruzyn );
                MyWoodPerDruzyna->find(it->first)->second.addToT( - it->second * wspolczynnikUdzialuDruzyn );
                it->second -= it->second * wspolczynnikUdzialuDruzyn;
              }
            
            for(map<string, int>::iterator it = teamSticks.begin();
            teamSticks.end() != it && zapotrzebowanie > 0;
            ++it)
              if(druzynyZbieraczy.end() != druzynyZbieraczy.find(it->first) && it->second > 0){
                --zapotrzebowanie;
                --it->second;
                MyWoodPerDruzyna->find(it->first)->second.addToS( -1 );
                MyWoodPerDruzyna->find(it->first)->second.addToT( -1 );
              }
            
          }
        }
        
      }
      
    }
    
  }
  
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

#endif