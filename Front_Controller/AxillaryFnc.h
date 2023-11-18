short colCon(long hexValue, char color)
{    
    if(color == 'R')        
      return ((hexValue >> 16) & 0xFF);    
    if(color == 'G')
        return ((hexValue >> 8) & 0xFF);
    if(color == 'B')
      return ((hexValue) & 0xFF);
}


long getMixCol(long col1, long col2, float br){  

  return long((col1 >> 16 & 0xff) + (int(col2 >> 16 & 0xFF) - int(col1 >> 16 & 0xFF)) * br) << 16 | long((col1 >> 8 & 0xff) + (int(col2 >> 8 & 0xFF) - int(col1 >> 8 & 0xFF)) * br) << 8 | long((col1 & 0xff) + (int(col2 & 0xFF) - int(col1 & 0xFF)) * br);
}



long getBrCol(long col, float br){
  
  return (long((((col >> 16) & 0xFF) * br)) << 16) | (long((((col >> 8) & 0xFF) * br)) << 8) | (long((col & 0xFF) * br));
}
