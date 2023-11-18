class LedInstruction{

  public:long fillStartTime, fillEndTime, startColor, endColor;
  
  public:LedInstruction *next = NULL;

  public:LedInstruction(long fst, long fet, long sc, long ec){

    fillStartTime = fst;
    fillEndTime = fet;
    startColor = sc;
    endColor = ec;
  }
};
