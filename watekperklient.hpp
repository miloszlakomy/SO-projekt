#ifndef WATEKPERKLIENT_HPP
#define WATEKPERKLIENT_HPP

// TODO refaktoryzacja - funkcje sprawdzajace czy zachodza bledy i wysylajace je do klientow

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

bool czyWspolrzedneSaPozaPlansza(const pair<int, int> & coords){
  
  if(coords.first < 0 || coords.second < 0)
    return true;
  
  int dummy_N = ParametryRozgrywki->getN();
  
  if(coords.first >= dummy_N || coords.second >= dummy_N)
    return true;
  
  return false;
  
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
      
NYI //TODO
      
    }
    else if("IGNITION" == komenda[0]){
      
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
          else if(Wyspy->find(zuczekDummy.getZuczekCoords())->second.getSticks() < ParametryRozgrywki->getSmin())
            sendError(handlerSocketu, 107);
          else{
            
            sendString(handlerSocketu, "OK");
            
            Wyspy->find(zuczekDummy.getZuczekCoords())->second.ignition(MyWoodPerDruzyna, ParametryRozgrywki);
            FireStatus->set(true);
            
            int dummy_L = L->get();
            
            if(dummy_L > 15)
              L->set(15);
            
          }
        }
      }
      
    }
    else if("MOVE" == komenda[0]){
      
      if(komenda.size() < 4) sendError(handlerSocketu, 3);
      else if(komenda.size() > 4) sendError(handlerSocketu, 4);
      else{
        
        int kodBledu;
        
        char * endptr;
        int ID = strtol(komenda[1].c_str(), &endptr, 0);
        if(*endptr)
          sendError(handlerSocketu, 3);
        else if(kodBledu = sprawdzZuczka(ID, nazwaDruzyny))
          sendError(handlerSocketu, kodBledu);
        else{
          
          int dX = strtol(komenda[2].c_str(), &endptr, 0);
          if(*endptr)
            sendError(handlerSocketu, 3);
          else{
          
            int dY = strtol(komenda[3].c_str(), &endptr, 0);
            
            if(*endptr)
              sendError(handlerSocketu, 3);
            else if( (1 != abs(dX) || 0 != abs(dY))
                  && (0 != abs(dX) || 1 != abs(dY)) )
              sendError(handlerSocketu, 102);
            else{
              
              Zuczek zuczekDummy = Zuczki->at(ID);
              
              if(czyWspolrzedneSaPozaPlansza(zuczekDummy.getZuczekCoords() + make_pair(dX, dY)))
                sendError(handlerSocketu, 103);
              else if(!zuczekDummy.makeBusy(1))
                sendError(handlerSocketu, 111);
              else{
                
                sendString(handlerSocketu, "OK");
                
                Zuczki->at(ID).enqueueMovement(make_pair(dX, dY));
                
              }
            }
          }
        }
      }
      
    }
    else if("TAKE" == komenda[0]){
      
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
          else if(!Zuczki->at(ID).makeBusy(1))
            sendError(handlerSocketu, 111);
          else{
            
            sendString(handlerSocketu, "OK");
            
            Wyspy->find(zuczekDummy.getZuczekCoords())->second.enqueueTaker(ID);
            
          }
        }
      }
      
    }
    else if("GIVE" == komenda[0]){
      
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
          else if(Zuczki->at(ID).getCarriedSticks() == 0)
            sendError(handlerSocketu, 108);
          else{
            
            sendString(handlerSocketu, "OK");
            
            int iloscOddawanychPatykow = Zuczki->at(ID).giveAllCarriedSticks(MyWoodPerDruzyna);
            
            Wyspy->find(zuczekDummy.getZuczekCoords())->second.leaveSticks(iloscOddawanychPatykow, nazwaDruzyny, MyWoodPerDruzyna, ParametryRozgrywki);
            
          }
        }
      }
      
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
      
      if(komenda.size() < 1) sendError(handlerSocketu, 3);
      else if(komenda.size() > 1) sendError(handlerSocketu, 4);
      else{
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

#endif