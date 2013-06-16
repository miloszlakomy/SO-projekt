#ifndef WRAPPERS_HPP
#define WRAPPERS_HPP



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

pair<int, int> operator+(const pair<int, int> & P1, const pair<int, int> & P2){
  
  return make_pair(
                    P1.first  + P2.first,
                    P1.second + P2.second
                  );
  
}

#endif