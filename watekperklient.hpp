#ifndef WATEKPERKLIENT_HPP
#define WATEKPERKLIENT_HPP



bool sprawdzIloscArgumentow                    (FILE * handlerSocketu, const vector<string> & komenda, int iloscArgumentow); // 3, 4
bool sprawdzIParsujStringDoInta                (FILE * handlerSocketu, const string & str, int & ID); // 3
bool sprawdzCzyZuczekIstniejeINalezyDoDruzyny  (FILE * handlerSocketu, int ID, const string & nazwaDruzyny); // 101, 105
bool sprawdzCzyZuczekJestNaWyspie              (FILE * handlerSocketu, int ID); // 104
bool sprawdzCzyNaWyspieMoznaRozpalicOgnisko    (FILE * handlerSocketu, int ID); // 107
bool sprawdzCzyWzgledneWspolrzedneSaJednostkowe(FILE * handlerSocketu, int dX, int dY); // 102
bool sprawdzCzyZuczekWyjdzieZaPlansze          (FILE * handlerSocketu, int ID, int dX, int dY); // 103
bool sprawdzCzyZuczekJestZajety                (FILE * handlerSocketu, int ID); // 109, 111
bool sprawdzCzyZuczekJestZajetyIZajmijGo       (FILE * handlerSocketu, int ID, int newBusyCounter); // 109, 111
bool sprawdzCzyZuczekMaPatyki                  (FILE * handlerSocketu, int ID); // 108

/////

void * watekPerKlient(void* _arg){
  
  FILE * handlerSocketu = ((pair<FILE *, string> *)_arg)->first;
  string nazwaDruzyny = ((pair<FILE *, string> *)_arg)->second;
  
  vector<string> komenda;
  
  for(;;){
    
    // TODO limit komend na turę
    
    komenda = myExplode(recieveString(handlerSocketu));
    
    if("DESCRIBE_WORLD" == komenda[0]){
      
      if(sprawdzIloscArgumentow(handlerSocketu, komenda, 0)){
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
      
      if(sprawdzIloscArgumentow(handlerSocketu, komenda, 0)){
        sendString(handlerSocketu, "OK");
        
        bool fsDummy = FireStatus->get();
        int lDummy = L->get();
        
        sendString(handlerSocketu,
                   string(fsDummy ? "BURNING" : "NONE") + " " +
                   NumberToString(lDummy)
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
      
      if(sprawdzIloscArgumentow(handlerSocketu, komenda, 0)){
        sendString(handlerSocketu, "OK");
        
        set<int> rpdDummy = RozbitkowiePerDruzyna->find(nazwaDruzyny)->second;
        
        string msg = "";
        int rozbitkowieCount = 0;
        
        for(set<int>::iterator it = rpdDummy.begin(); rpdDummy.end() != it; ++it)
          if(Zuczki->at(*it).getUtopiony() == false){
            msg += NumberToString(*it) + " ";
            ++rozbitkowieCount;
          }
        
        msg.erase(msg.size()-1);
        
        sendString(handlerSocketu,
                   NumberToString(rozbitkowieCount)
                  );
        
        sendString(handlerSocketu,
                   msg
                  );
        
      }
    }
    else if("LIST_RAFTS" == komenda[0]){
      
      sendError(handlerSocketu, 5);
      
NYI //TODO
      
    }
    else if("IGNITION" == komenda[0]){
      
      int ID;
      
      if(sprawdzIloscArgumentow                  (handlerSocketu, komenda, 1)
      && sprawdzIParsujStringDoInta              (handlerSocketu, komenda[1], ID)
      && sprawdzCzyZuczekIstniejeINalezyDoDruzyny(handlerSocketu, ID, nazwaDruzyny)
      && sprawdzCzyZuczekJestZajety              (handlerSocketu, ID)
      && sprawdzCzyZuczekJestNaWyspie            (handlerSocketu, ID)
      && sprawdzCzyNaWyspieMoznaRozpalicOgnisko  (handlerSocketu, ID)
      ){
        sendString(handlerSocketu, "OK");
        
        pair<int, int> coordsDummy = Zuczki->at(ID).getZuczekCoords();
        
        Wyspy->find(coordsDummy)->second.ignition(MyWoodPerDruzyna, ParametryRozgrywki);
        FireStatus->set(true);
        
        int lDummy = L->get();
        
        if(lDummy > 15)
          L->set(15);
      }
    }
    else if("MOVE" == komenda[0]){
      
      int ID;
      int dX;
      int dY;
      
      if(sprawdzIloscArgumentow                    (handlerSocketu, komenda, 3)
      && sprawdzIParsujStringDoInta                (handlerSocketu, komenda[1], ID)
      && sprawdzIParsujStringDoInta                (handlerSocketu, komenda[2], dX)
      && sprawdzIParsujStringDoInta                (handlerSocketu, komenda[3], dY)
      && sprawdzCzyZuczekIstniejeINalezyDoDruzyny  (handlerSocketu, ID, nazwaDruzyny)
      && sprawdzCzyWzgledneWspolrzedneSaJednostkowe(handlerSocketu, dX, dY)
      && sprawdzCzyZuczekWyjdzieZaPlansze          (handlerSocketu, ID, dX, dY)
      && sprawdzCzyZuczekJestZajetyIZajmijGo       (handlerSocketu, ID, 1)
      ){
        sendString(handlerSocketu, "OK");
        
        Zuczki->at(ID).enqueueMovement(make_pair(dX, dY));
      }
    }
    else if("TAKE" == komenda[0]){
      
      int ID;
      
      if(sprawdzIloscArgumentow                  (handlerSocketu, komenda, 1)
      && sprawdzIParsujStringDoInta              (handlerSocketu, komenda[1], ID)
      && sprawdzCzyZuczekIstniejeINalezyDoDruzyny(handlerSocketu, ID, nazwaDruzyny)
      && sprawdzCzyZuczekJestNaWyspie            (handlerSocketu, ID)
      && sprawdzCzyZuczekJestZajetyIZajmijGo     (handlerSocketu, ID, 1)
      ){
        sendString(handlerSocketu, "OK");
        
        pair<int, int> coordsDummy = Zuczki->at(ID).getZuczekCoords();
        Wyspy->find(coordsDummy)->second.enqueueTaker(ID);
      }
    }
    else if("GIVE" == komenda[0]){
      
      int ID;
      
      if(sprawdzIloscArgumentow                  (handlerSocketu, komenda, 1)
      && sprawdzIParsujStringDoInta              (handlerSocketu, komenda[1], ID)
      && sprawdzCzyZuczekIstniejeINalezyDoDruzyny(handlerSocketu, ID, nazwaDruzyny)
      && sprawdzCzyZuczekJestZajety              (handlerSocketu, ID)
      && sprawdzCzyZuczekJestNaWyspie            (handlerSocketu, ID)
      && sprawdzCzyZuczekMaPatyki                (handlerSocketu, ID)
      ){
        sendString(handlerSocketu, "OK");
        
        int iloscOddawanychPatykow = Zuczki->at(ID).giveAllCarriedSticks(MyWoodPerDruzyna);
        
        pair<int, int> coordsDummy = Zuczki->at(ID).getZuczekCoords();
        Wyspy->find(coordsDummy)->second.leaveSticks(iloscOddawanychPatykow, nazwaDruzyny, MyWoodPerDruzyna, ParametryRozgrywki);
      }
    }
    else if("GUARD" == komenda[0]){
      
      sendError(handlerSocketu, 5);
      
NYI //TODO
      
    }
    else if("STOP_GUARDING" == komenda[0]){
      
      sendError(handlerSocketu, 5);
      
NYI //TODO
      
    }
    else if("LIST_WOOD" == komenda[0]){
      
      int ID;
      
      if(sprawdzIloscArgumentow                  (handlerSocketu, komenda, 1)
      && sprawdzIParsujStringDoInta              (handlerSocketu, komenda[1], ID)
      && sprawdzCzyZuczekIstniejeINalezyDoDruzyny(handlerSocketu, ID, nazwaDruzyny)
      && sprawdzCzyZuczekJestNaWyspie            (handlerSocketu, ID)
      ){
        sendString(handlerSocketu, "OK");
        
        pair<int, int> coordsDummy = Zuczki->at(ID).getZuczekCoords();
        ListWoodResult listWoodResult(coordsDummy);
        Wyspy.runFunction(getListWoodResultFn, (void*)&listWoodResult);
        
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
    else if("INFO" == komenda[0]){
      
      int ID;
      
      if(sprawdzIloscArgumentow                  (handlerSocketu, komenda, 1)
      && sprawdzIParsujStringDoInta              (handlerSocketu, komenda[1], ID)
      && sprawdzCzyZuczekIstniejeINalezyDoDruzyny(handlerSocketu, ID, nazwaDruzyny)
      ){
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
          
          int nDummy = ParametryRozgrywki->getN();
          
          if(fieldCoords.first < 0       || fieldCoords.second < 0
          || fieldCoords.first >= nDummy || fieldCoords.second >= nDummy)
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
    else if("BUILD" == komenda[0]){
      
      sendError(handlerSocketu, 5);
      
NYI //TODO
      
    }
    else if("ABANDON" == komenda[0]){
      
      sendError(handlerSocketu, 5);
      
NYI //TODO
      
    }
    else if("TAKE_OVER" == komenda[0]){
      
      sendError(handlerSocketu, 5);
      
NYI //TODO
      
    }
    else if("DRY" == komenda[0]){
      
      sendError(handlerSocketu, 5);
      
NYI //TODO
      
    }
    else if("MY_WOOD" == komenda[0]){
      
      if(sprawdzIloscArgumentow(handlerSocketu, komenda, 0)){
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
      
      if(sprawdzIloscArgumentow(handlerSocketu, komenda, 0)){
        sendString(handlerSocketu, "OK");
        
        double czasOczekiwania = Tstart->get() + ParametryRozgrywki->getT() - czasRzeczywisty();
        
        sendString(handlerSocketu,
                   "WAITING " + NumberToString(czasOczekiwania)
                  );
        
        mySleep(czasOczekiwania);
        
        sendString(handlerSocketu, "OK");
      }
    }
    else
      sendError(handlerSocketu, 2);
    
  }
  
} // koniec watku per klient

/////

bool sprawdzIloscArgumentow(FILE * handlerSocketu, const vector<string> & komenda, int iloscArgumentow){ // 3, 4
  
  if(komenda.size() < 1 + iloscArgumentow){
    sendError(handlerSocketu, 3);
    return false;
  }
  
  if(komenda.size() > 1 + iloscArgumentow){
    sendError(handlerSocketu, 4);
    return false;
  }
  
  return true;
}

bool sprawdzIParsujStringDoInta(FILE * handlerSocketu, const string & str, int & ID){ // 3
  
  char * endptr;
  ID = strtol(str.c_str(), &endptr, 0);
  
  if(*endptr){
    sendError(handlerSocketu, 3);
    return false;
  }
  
  return true;
}

bool sprawdzCzyZuczekIstniejeINalezyDoDruzyny(FILE * handlerSocketu, int ID, const string & nazwaDruzyny){ // 101, 105
  
  int ZuczkiSizeDummy = Zuczki->size();
  set<int> rpdDummy = RozbitkowiePerDruzyna->find(nazwaDruzyny)->second;
  
  if(0 > ID || ID >= ZuczkiSizeDummy || rpdDummy.end() == rpdDummy.find(ID)){
    sendError(handlerSocketu, 101);
    return false;
  }
  
  bool utopionyDummy = Zuczki->at(ID).getUtopiony();
  
  if(utopionyDummy){
    sendError(handlerSocketu, 105);
    return false;
  }
  
  return true;
}

bool sprawdzCzyZuczekJestNaWyspie(FILE * handlerSocketu, int ID){ // 104
  
  pair<int, int> coordsDummy = Zuczki->at(ID).getZuczekCoords();
  bool wyspaDummy = Mapa->at(coordsDummy.first).at(coordsDummy.second).getWyspa();
  
  if(!wyspaDummy){
    sendError(handlerSocketu, 104);
    return false;
  }
  
  return true;
}

bool sprawdzCzyNaWyspieMoznaRozpalicOgnisko(FILE * handlerSocketu, int ID){ // 107
  
  pair<int, int> coordsDummy = Zuczki->at(ID).getZuczekCoords();
  int sticksDummy = Wyspy->find(coordsDummy)->second.getSticks();
  int SminDummy = ParametryRozgrywki->getSmin();
  
  if(sticksDummy < SminDummy){
    sendError(handlerSocketu, 107);
    return false;
  }
  
  return true;
}

bool sprawdzCzyWzgledneWspolrzedneSaJednostkowe(FILE * handlerSocketu, int dX, int dY){ // 102
  
  if( (1 != abs(dX) || 0 != abs(dY))
   && (0 != abs(dX) || 1 != abs(dY)) ){
     sendError(handlerSocketu, 102);
     return false;
  }
  
  return true;
}

bool sprawdzCzyZuczekWyjdzieZaPlansze(FILE * handlerSocketu, int ID, int dX, int dY){ // 103
  
  pair<int, int> coordsDummy = Zuczki->at(ID).getZuczekCoords()
                               + make_pair(dX, dY);
  int nDummy = ParametryRozgrywki->getN();
  
  if(coordsDummy.first < 0       || coordsDummy.second < 0
  || coordsDummy.first >= nDummy || coordsDummy.second >= nDummy){
    sendError(handlerSocketu, 103);
    return false;
  }
  
  return true;
}

bool sprawdzCzyZuczekJestZajety(FILE * handlerSocketu, int ID){ // 109, 111
  
  int busyCounterDummy = Zuczki->at(ID).getBusyCounter();
  
  if(busyCounterDummy != 0){
    if(busyCounterDummy < 0)
      sendError(handlerSocketu, 109);
    else
      sendError(handlerSocketu, 111);
    
    return false;
  }
  
  return true;
}

bool sprawdzCzyZuczekJestZajetyIZajmijGo(FILE * handlerSocketu, int ID, int newBusyCounter){ // 109, 111
  
  bool makeBusyRet = Zuczki->at(ID).makeBusy(newBusyCounter);
  
  if(makeBusyRet == false){
    
    int busyCounterDummy = Zuczki->at(ID).getBusyCounter();
    
    if(busyCounterDummy < 0)
      sendError(handlerSocketu, 109);
    else
      sendError(handlerSocketu, 111);
    
    return false;
  }
  
  return true;
}

bool sprawdzCzyZuczekMaPatyki (FILE * handlerSocketu, int ID){ // 108
  
  int csDummy = Zuczki->at(ID).getCarriedSticks();
  
  if(0 == csDummy){
    sendError(handlerSocketu, 108);
    return false;
  }
  
  return true;
}

#endif