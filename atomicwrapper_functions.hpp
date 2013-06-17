#ifndef ATOMICWRAPPER_FUNCTIONS_HPP
#define ATOMICWRAPPER_FUNCTIONS_HPP


void generujWynikiFn(map<string, MyWood> & unwrapped_MyWoodPerDruzyna, void *){
  
  logger << endl << "Wyniki:" << endl;
  
  for(map<string, MyWood>::iterator it = unwrapped_MyWoodPerDruzyna.begin();
      unwrapped_MyWoodPerDruzyna.end() != it;
      ++it)
    logger << it->first << ' ' << it->second.getT() << endl;
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
  
  string buffer2;
  
  buffer2 += clearScreen;
  for(int i=0;i<buffer.size();++i){
    for(int j=0;j<buffer[i].size();++j){
      switch(buffer[i][j]){
        case '.': break;
        case '#': buffer2 += setTextColor_Green; break;
        case '(':
        case ')':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
                  buffer2 += setTextColor_White; break;
        default:  buffer2 += setTextColor_Red;
      }
      buffer2 += buffer[i][j];
      
      if('.' != buffer[i][j])
       buffer2 += setTextColor_Blue;
    }
    buffer2 += "\n";
  }
  buffer2 += setTextColor_White;
  
  cout << buffer2 << flush;
}

#endif