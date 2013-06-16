#ifndef ATOMICWRAPPER_FUNCTIONS_HPP
#define ATOMICWRAPPER_FUNCTIONS_HPP

void zerujMapeFn(vector<vector<Pole> > & unwrapped_Mapa, void *){
  
  int dummy_N    = ParametryRozgrywki->getN();
  
  unwrapped_Mapa = vector<vector<Pole> >(
                                          dummy_N,
                                          vector<Pole>(dummy_N)
                                        );
  
}

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
    unwrapped_Wyspy.insert(make_pair(newIslandsCoords, Wyspa(newIslandsCoords, iloscPatykowNaNowejWyspie)));
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

void getListWoodResultFn(map<pair<int, int>, Wyspa> & unwrapped_Wyspy, void * _listWoodResult){
  
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

void zbieraniePatykowFn(map<pair<int, int>, Wyspa> & unwrapped_Wyspy, void *){
  
  for(map<pair<int, int>, Wyspa>::iterator it = unwrapped_Wyspy.begin();
      unwrapped_Wyspy.end() != it;
      ++it)
    it->second.letTakersTake(Mapa, Zuczki, MyWoodPerDruzyna);
  
}

void displayWorldFn(vector<vector<Pole> > & unwrapped_Mapa, void *){
  
  vector<string> buffer(1);
  
  for(int y=0;y<unwrapped_Mapa.size();++y){
    for(int x=0;x<unwrapped_Mapa.size();++x){
      if(unwrapped_Mapa[x][y].getWyspa()){
        
        int sticks = Wyspy->find(make_pair(x,y))->second.getSticks();
        
        buffer[buffer.size()-1] += "#(" + NumberToString(sticks) + ")";
        x += NumberToString(sticks).size()+2;
      }
      else
        buffer[buffer.size()-1] += ".";
    }
    buffer.push_back("");
  }
  
  for(int y=0;y<unwrapped_Mapa.size();++y)
    for(int x=0;x<unwrapped_Mapa.size();++x)
      if(unwrapped_Mapa[x][y].getWyspa())
        buffer[y][x] = '#';
  
  int dummy_ZuczkiSize = Zuczki->size();
  for(int i=0;i<dummy_ZuczkiSize;++i)
    if(Zuczki->at(i).getUtopiony() == false){
      pair<int, int> coords = Zuczki->at(i).getZuczekCoords();
  //     buffer[coords.second][coords.first] = NumberToString(Zuczki->at(i).getCarriedSticks()%10)[0];
      buffer[coords.second][coords.first] = hashStringToLetter(Zuczki->at(i).getNazwaDruzyny());
    }
  
  for(int i=0;i<buffer.size();++i)
    cout << buffer[i] << endl;
  
}

#endif