/*
Roopam Taneja
22125030
CSE - O3
*/
#include "pass1.cpp"

using namespace std;

ifstream interFile;
ofstream errorFile,objectFile,ListingFile;

ofstream printtab ;
string writestring ;
  
bool isComment;
string label,opcode,operand,comment;
string operand1,operand2;

int lineNumber,blockNumber,address,startAddress;
string objectCode, writeData, currentRecord, modificationRecord="M^", endRecord, write_R_Data, write_D_Data,currentSectName="DEFAULT";

int program_counter, base_register_value, currentTextRecordLength;
bool nobase;

string readTillTab(string data,int& index){
  string tempBuffer = "";

  while(index<data.length() && data[index] != '\t'){
    tempBuffer += data[index];
    index++;
  }
  index++;
  if(tempBuffer==" "){
    tempBuffer="-1" ;
  }
  return tempBuffer;
}
bool readIntermediateFile(ifstream& readFile,bool& isComment, int& lineNumber, int& address, int& blockNumber, string& label, string& opcode, string& operand, string& comment){
  string fileLine="";
  string tempBuffer="";
  bool tempStatus;
  int index=0;
  if(!getline(readFile, fileLine)){
    return false;
  }
  lineNumber = stoi(readTillTab(fileLine,index));

  isComment = (fileLine[index]=='.')?true:false;
  if(isComment){
    readFirstNonWhiteSpace(fileLine,index,tempStatus,comment,true);
    return true;
  }
  address = hexToInt(readTillTab(fileLine,index));
  tempBuffer = readTillTab(fileLine,index);
  if(tempBuffer == " "){
    blockNumber = -1;
  }
  else{
    blockNumber = stoi(tempBuffer);
  }
  label = readTillTab(fileLine,index);
  if(label=="-1"){
    label=" " ;
  }
  opcode = readTillTab(fileLine,index);
  if(opcode=="BYTE"){
    readByteOperand(fileLine,index,tempStatus,operand);
  }
  else{
    operand = readTillTab(fileLine,index);
    if(operand=="-1"){
      operand=" " ;
    }
  }
  readFirstNonWhiteSpace(fileLine,index,tempStatus,comment,true);  
  return true;
}

string createObjectCodeFormat34(){
  string objcode;
  int halfBytes;
  halfBytes = (getFlagFormat(opcode)=='+')?5:3;

  if(getFlagFormat(operand)=='#'){
    if(operand.substr(operand.length()-2,2)==",X"){
      writeData = "Line: "+to_string(lineNumber)+" Index based addressing not supported with Indirect addressing";
      writeToFile(errorFile,writeData);
      objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
      objcode += (halfBytes==5)?"100000":"0000";
      return objcode;
    }

    string tempOperand = operand.substr(1,operand.length()-1);
    if(if_all_num(tempOperand)||((SYMTAB[tempOperand].exists=='y')&&(SYMTAB[tempOperand].relative==0))){
      int immediateValue;

      if(if_all_num(tempOperand)){
        immediateValue = stoi(tempOperand);
      }
      else{
        immediateValue = hexToInt(SYMTAB[tempOperand].address);
      }
      
      if(immediateValue>=(1<<4*halfBytes)){
        writeData = "Line: "+to_string(lineNumber)+" Immediate value exceeds format limit";
        writeToFile(errorFile,writeData);
        objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
        objcode += (halfBytes==5)?"100000":"0000";
      }
      else{
        objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
        objcode += (halfBytes==5)?'1':'0';
        objcode += intToHex(immediateValue,halfBytes);
      }
      return objcode;
    }
    else if(SYMTAB[tempOperand].exists=='n') {
     
      if(halfBytes==3) { 
         writeData += " : Symbol doesn't exists. Found " + tempOperand; 
      writeToFile(errorFile,writeData);
      objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
      objcode += (halfBytes==5)?"100000":"0000";
      return objcode;
    }
            if(SYMTAB[tempOperand].exists=='y') {
             objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
             objcode += "100000";

             modificationRecord += "M^" + intToHex(address+1,6) + '^';
             modificationRecord += "05+";
             modificationRecord += '\n';       

            return objcode ;
         }
    }
    else{
      int operandAddress = hexToInt(SYMTAB[tempOperand].address) + hexToInt(BLOCKS[BLocksNumToName[SYMTAB[tempOperand].blockNumber]].startAddress);

      if(halfBytes==5){
        objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
        objcode += '1';
        objcode += intToHex(operandAddress,halfBytes);

        
        modificationRecord += "M^" + intToHex(address+1,6) + '^';
        modificationRecord += (halfBytes==5)?"05":"03";
        modificationRecord += '\n';

        return objcode;
      }
      program_counter = address + hexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress);
      program_counter += (halfBytes==5)?4:3;
      int relativeAddress = operandAddress - program_counter;

     
      if(relativeAddress>=(-2048) && relativeAddress<=2047){
        objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
        objcode += '2';
        objcode += intToHex(relativeAddress,halfBytes);
        return objcode;
      }
      
      if(!nobase){
        relativeAddress = operandAddress - base_register_value;
        if(relativeAddress>=0 && relativeAddress<=4095){
          objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
          objcode += '4';
          objcode += intToHex(relativeAddress,halfBytes);
          return objcode;
        }
      }
      
      if(operandAddress<=4095){
        objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
        objcode += '0';
        objcode += intToHex(operandAddress,halfBytes);

        modificationRecord += "M^" + intToHex(address+1+hexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress),6) + '^';
        modificationRecord += (halfBytes==5)?"05":"03";
        modificationRecord += '\n';

        return objcode;
      }
    }
  }
  else if(getFlagFormat(operand)=='@'){
    string tempOperand = operand.substr(1,operand.length()-1);
    if(tempOperand.substr(tempOperand.length()-2,2)==",X" || SYMTAB[tempOperand].exists=='n' ){
      
      writeData = "Line "+to_string(lineNumber);
      writeData += (SYMTAB[tempOperand].exists=='n')?": Symbol doesn't exists":" Index based addressing not supported with Indirect addressing";
      writeToFile(errorFile,writeData);
      objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
      objcode += (halfBytes==5)?"100000":"0000";
      return objcode;
   
}
    int operandAddress = hexToInt(SYMTAB[tempOperand].address) + hexToInt(BLOCKS[BLocksNumToName[SYMTAB[tempOperand].blockNumber]].startAddress);
    program_counter = address + hexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress);
    program_counter += (halfBytes==5)?4:3;

    if(halfBytes==3){
      int relativeAddress = operandAddress - program_counter;
      if(relativeAddress>=(-2048) && relativeAddress<=2047){
        objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
        objcode += '2';
        objcode += intToHex(relativeAddress,halfBytes);
        return objcode;
      }

      if(!nobase){
        relativeAddress = operandAddress - base_register_value;
        if(relativeAddress>=0 && relativeAddress<=4095){
          objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
          objcode += '4';
          objcode += intToHex(relativeAddress,halfBytes);
          return objcode;
        }
      }

      if(operandAddress<=4095){
        objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
        objcode += '0';
        objcode += intToHex(operandAddress,halfBytes);

        
        modificationRecord += "M^" + intToHex(address+1+hexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress),6) + '^';
        modificationRecord += (halfBytes==5)?"05":"03";
        modificationRecord += '\n';

        return objcode;
      }
    }
    else{
      objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
      objcode += '1';
      objcode += intToHex(operandAddress,halfBytes);

      
      modificationRecord += "M^" + intToHex(address+1+hexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress),6) + '^';
      modificationRecord += (halfBytes==5)?"05":"03";
      modificationRecord += '\n';

      return objcode;
    }

    writeData = "Line: "+to_string(lineNumber);
    writeData += "Can't fit into program counter based or base register based addressing.";
    writeToFile(errorFile,writeData);
    objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
    objcode += (halfBytes==5)?"100000":"0000";
    return objcode;
  }
  else if(getFlagFormat(operand)=='='){
    string tempOperand = operand.substr(1,operand.length()-1);

    if(tempOperand=="*"){
      tempOperand = "X'" + intToHex(address,6) + "'";

      modificationRecord += "M^" + intToHex(hexToInt(LITTAB[tempOperand].address)+hexToInt(BLOCKS[BLocksNumToName[LITTAB[tempOperand].blockNumber]].startAddress),6) + '^';
      modificationRecord += intToHex(6,2);
      modificationRecord += '\n';
    }

    if(LITTAB[tempOperand].exists=='n'){
      writeData = "Line "+to_string(lineNumber)+" : Symbol doesn't exists. Found " + tempOperand;
      writeToFile(errorFile,writeData);

      objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
      objcode += (halfBytes==5)?"000":"0";
      objcode += "000";
      return objcode;
    }

    int operandAddress = hexToInt(LITTAB[tempOperand].address) + hexToInt(BLOCKS[BLocksNumToName[LITTAB[tempOperand].blockNumber]].startAddress);
    program_counter = address + hexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress);
    program_counter += (halfBytes==5)?4:3;

    if(halfBytes==3){
      int relativeAddress = operandAddress - program_counter;
      if(relativeAddress>=(-2048) && relativeAddress<=2047){
        objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
        objcode += '2';
        objcode += intToHex(relativeAddress,halfBytes);
        return objcode;
      }

      if(!nobase){
        relativeAddress = operandAddress - base_register_value;
        if(relativeAddress>=0 && relativeAddress<=4095){
          objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
          objcode += '4';
          objcode += intToHex(relativeAddress,halfBytes);
          return objcode;
        }
      }

      if(operandAddress<=4095){
        objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
        objcode += '0';
        objcode += intToHex(operandAddress,halfBytes);

       
        modificationRecord += "M^" + intToHex(address+1+hexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress),6) + '^';
        modificationRecord += (halfBytes==5)?"05":"03";
        modificationRecord += '\n';

        return objcode;
      }
    }
    else{
      objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
      objcode += '1';
      objcode += intToHex(operandAddress,halfBytes);

     
      modificationRecord += "M^" + intToHex(address+1+hexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress),6) + '^';
      modificationRecord += (halfBytes==5)?"05":"03";
      modificationRecord += '\n';

      return objcode;
    }

    writeData = "Line: "+to_string(lineNumber);
    writeData += "Can't fit into program counter based or base register based addressing.";
    writeToFile(errorFile,writeData);
    objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
    objcode += (halfBytes==5)?"100":"0";
    objcode += "000";

    return objcode;
  }
  else{
    int xbpe=0;
    string tempOperand = operand;
    if(operand.substr(operand.length()-2,2)==",X"){
      tempOperand = operand.substr(0,operand.length()-2);
      xbpe = 8;
    }

   if(SYMTAB[tempOperand].exists=='n'){
      writeData = "Line "+to_string(lineNumber)+" : Symbol doesn't exists. Found " + tempOperand;
      writeToFile(errorFile,writeData);

      objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
      objcode += (halfBytes==5)?(intToHex(xbpe+1,1)+"00"):intToHex(xbpe,1);
      objcode += "000";
      return objcode;
    }
else{
  
    int operandAddress = hexToInt(SYMTAB[tempOperand].address) + hexToInt(BLOCKS[BLocksNumToName[SYMTAB[tempOperand].blockNumber]].startAddress);
    program_counter = address + hexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress);
    program_counter += (halfBytes==5)?4:3;

    if(halfBytes==3){
      int relativeAddress = operandAddress - program_counter;
      if(relativeAddress>=(-2048) && relativeAddress<=2047){
        objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
        objcode += intToHex(xbpe+2,1);
        objcode += intToHex(relativeAddress,halfBytes);
        return objcode;
      }

      if(!nobase){
        relativeAddress = operandAddress - base_register_value;
        if(relativeAddress>=0 && relativeAddress<=4095){
          objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
          objcode += intToHex(xbpe+4,1);
          objcode += intToHex(relativeAddress,halfBytes);
          return objcode;
        }
      }

      if(operandAddress<=4095){
        objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
        objcode += intToHex(xbpe,1);
        objcode += intToHex(operandAddress,halfBytes);

       
        modificationRecord += "M^" + intToHex(address+1+hexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress),6) + '^';
        modificationRecord += (halfBytes==5)?"05":"03";
        modificationRecord += '\n';

        return objcode;
      }
    }
    else{
      objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
      objcode += intToHex(xbpe+1,1);
      objcode += intToHex(operandAddress,halfBytes);

     
      modificationRecord += "M^" + intToHex(address+1+hexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress),6) + '^';
      modificationRecord += (halfBytes==5)?"05":"03";
      modificationRecord += '\n';

      return objcode;
    }

    writeData = "Line: "+to_string(lineNumber);
    writeData += "Can't fit into program counter based or base register based addressing.";
    writeToFile(errorFile,writeData);
    objcode = intToHex(hexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
    objcode += (halfBytes==5)?(intToHex(xbpe+1,1)+"00"):intToHex(xbpe,1);
    objcode += "000";

    return objcode;
  }}
}

void writeTextRecord(bool lastRecord=false){
  if(lastRecord){
    if(currentRecord.length()>0){//Write last text record
      writeData = intToHex(currentRecord.length()/2,2) + '^' + currentRecord;
      writeToFile(objectFile,writeData);
      currentRecord = "";
    }
    return;
  }
  if(objectCode != ""){
    if(currentRecord.length()==0){
      writeData = "T^" + intToHex(address+hexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress),6) + '^';
      writeToFile(objectFile,writeData,false);
    }
   
    if((currentRecord + objectCode).length()>60){
      
      writeData = intToHex(currentRecord.length()/2,2) + '^' + currentRecord;
      writeToFile(objectFile,writeData);

     
      currentRecord = "";
      writeData = "T^" + intToHex(address+hexToInt(BLOCKS[BLocksNumToName[blockNumber]].startAddress),6) + '^';
      writeToFile(objectFile,writeData,false);
    }

    currentRecord += objectCode;
  }
  else{
    if(opcode=="START"||opcode=="END"||opcode=="BASE"||opcode=="NOBASE"||opcode=="LTORG"||opcode=="ORG"||opcode=="EQU"){
     
    }
    else{
      
      if(currentRecord.length()>0){
        writeData = intToHex(currentRecord.length()/2,2) + '^' + currentRecord;
        writeToFile(objectFile,writeData);
      }
      currentRecord = "";
    }
  }
}

void writeEndRecord(bool write=true){
  if(write){
    if(endRecord.length()>0){
      writeToFile(objectFile,endRecord);
     
    }
    else{
      writeEndRecord(false);
    }
  }
    int firstExecutableAddress;
   
      firstExecutableAddress = hexToInt(SYMTAB[firstExecutable_Sec].address);
    
    endRecord = "E^" + intToHex(firstExecutableAddress,6)+"\n";
}

void pass2(){
  string tempBuffer;
  interFile.open("intermediate_"+fileName);
  if(!interFile){
    cout<<"Unable to open file: intermediate_"<<fileName<<endl;
    exit(1);
  }
  getline(interFile, tempBuffer);

  objectFile.open("object_"+fileName);
  if(!objectFile){
    cout<<"Unable to open file: object_"<<fileName<<endl;
    exit(1);
  }

  ListingFile.open("listing_"+fileName);
  if(!ListingFile){
    cout<<"Unable to open file: listing_"<<fileName<<endl;
    exit(1);
  }
  writeToFile(ListingFile,"Line\tAddress\tLabel\tOPCODE\tOPERAND\tObjectCode\tComment");

  errorFile.open("error_"+fileName,fstream::app);
  if(!errorFile){
    cout<<"Unable to open file: error_"<<fileName<<endl;
    exit(1);
  }
  writeToFile(errorFile,"\n\n************PASS2************");
  objectCode = "";
  currentTextRecordLength=0;
  currentRecord = "";
  modificationRecord = "";
  blockNumber = 0;
  nobase = true;

  readIntermediateFile(interFile,isComment,lineNumber,address,blockNumber,label,opcode,operand,comment);
  while(isComment){
    writeData = to_string(lineNumber) + "\t" + comment;
    writeToFile(ListingFile,writeData);
    readIntermediateFile(interFile,isComment,lineNumber,address,blockNumber,label,opcode,operand,comment);
  }

  if(opcode=="START"){
    startAddress = address;
    writeData = to_string(lineNumber) + "\t" + intToHex(address) + "\t" + to_string(blockNumber) + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + objectCode +"\t" + comment;
    writeToFile(ListingFile,writeData);
  }
  else{
    label = "";
    startAddress = 0;
    address = 0;
  }
  
  writeData = "H^"+expandString(label,6,' ',true)+'^'+intToHex(address,6)+'^'+intToHex(program_length,6);
  writeToFile(objectFile,writeData);
 
  readIntermediateFile(interFile,isComment,lineNumber,address,blockNumber,label,opcode,operand,comment);
  currentTextRecordLength  = 0;

  while(opcode!="END"){
    if(!isComment){
      if(OPTAB[getRealOpcode(opcode)].exists=='y'){
        if(OPTAB[getRealOpcode(opcode)].format==1){
          objectCode = OPTAB[getRealOpcode(opcode)].opcode;
        }
        else if(OPTAB[getRealOpcode(opcode)].format==2){
          operand1 = operand.substr(0,operand.find(','));
          operand2 = operand.substr(operand.find(',')+1,operand.length()-operand.find(',') -1 );

          if(operand2==operand){
            if(getRealOpcode(opcode)=="SVC"){
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + intToHex(stoi(operand1),1) + '0';
            }
            else if(REGTAB[operand1].exists=='y'){
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + REGTAB[operand1].num + '0';
            }
            else{
              objectCode = getRealOpcode(opcode) + '0' + '0';
              writeData = "Line: "+to_string(lineNumber)+" Invalid Register name";
              writeToFile(errorFile,writeData);
            }
          }
          else{
            if(REGTAB[operand1].exists=='n'){
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + "00";
              writeData = "Line: "+to_string(lineNumber)+" Invalid Register name";
              writeToFile(errorFile,writeData);
            }
            else if(getRealOpcode(opcode)=="SHIFTR" || getRealOpcode(opcode)=="SHIFTL"){
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + REGTAB[operand1].num + intToHex(stoi(operand2),1);
            }
            else if(REGTAB[operand2].exists=='n'){
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + "00";
              writeData = "Line: "+to_string(lineNumber)+" Invalid Register name";
              writeToFile(errorFile,writeData);
            }
            else{
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + REGTAB[operand1].num + REGTAB[operand2].num;
            }
          }
        }
        else if(OPTAB[getRealOpcode(opcode)].format==3){
          if(getRealOpcode(opcode)=="RSUB"){
            objectCode = OPTAB[getRealOpcode(opcode)].opcode;
            objectCode += (getFlagFormat(opcode)=='+')?"000000":"0000";
          }
          else{
            objectCode = createObjectCodeFormat34();
          }
        }
      }
      else if(opcode=="BYTE"){
        if(operand[0]=='X'){
          objectCode = operand.substr(2,operand.length()-3);
        }
        else if(operand[0]=='C'){
          objectCode = stringToHexString(operand.substr(2,operand.length()-3));
        }
      }
      else if(label=="*"){
        if(opcode[1]=='C'){
          objectCode = stringToHexString(opcode.substr(3,opcode.length()-4));
        }
        else if(opcode[1]=='X'){
          objectCode = opcode.substr(3,opcode.length()-4);
        }
      }
      else if(opcode=="WORD"){
        objectCode = intToHex(stoi(operand),6);
      }
      else if(opcode=="BASE"){
        if(SYMTAB[operand].exists=='y'){
          base_register_value = hexToInt(SYMTAB[operand].address) + hexToInt(BLOCKS[BLocksNumToName[SYMTAB[operand].blockNumber]].startAddress);
          nobase = false;
        }
        else{
          writeData = "Line "+to_string(lineNumber)+" : Symbol doesn't exists. Found " + operand;
          writeToFile(errorFile,writeData);
        }
        objectCode = "";
      }
      else if(opcode=="NOBASE"){
        if(nobase){
          writeData = "Line "+to_string(lineNumber)+": Assembler wasn't using base addressing";
          writeToFile(errorFile,writeData);
        }
        else{
          nobase = true;
        }
        objectCode = "";
      }
      else{
        objectCode = "";
      }
      
      writeTextRecord();

      if(blockNumber==-1 && address!=-1){
        writeData = to_string(lineNumber) + "\t" + intToHex(address) + "\t" + " " + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + objectCode +"\t" + comment;
      }
      else{ writeData = to_string(lineNumber) + "\t" + intToHex(address) + "\t" + to_string(blockNumber) + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + objectCode +"\t" + comment;
      }
    }
    else{
      writeData = to_string(lineNumber) + "\t" + comment;
    }
    writeToFile(ListingFile,writeData);//Write listing line
    readIntermediateFile(interFile,isComment,lineNumber,address,blockNumber,label,opcode,operand,comment);//Read next line
    objectCode = "";
  }
  writeTextRecord();

  
  writeEndRecord(false);
  
   if(!isComment){
  writeData = to_string(lineNumber) + "\t" + intToHex(address) + "\t" + " " + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + "" +"\t" + comment;
}else{
  writeData = to_string(lineNumber) + "\t" + comment;
 }
  writeToFile(ListingFile,writeData);


  while(readIntermediateFile(interFile,isComment,lineNumber,address,blockNumber,label,opcode,operand,comment)){
    if(label=="*"){
      if(opcode[1]=='C'){
        objectCode = stringToHexString(opcode.substr(3,opcode.length()-4));
      }
      else if(opcode[1]=='X'){
        objectCode = opcode.substr(3,opcode.length()-4);
      }
      writeTextRecord();
    }
    writeData = to_string(lineNumber) + "\t" + intToHex(address) + "\t" + to_string(blockNumber) + label + "\t" + opcode + "\t" + operand + "\t" + objectCode +"\t" + comment;
    writeToFile(ListingFile,writeData);
  }
  
writeTextRecord(true);
if(!isComment){
  
  writeToFile(objectFile,modificationRecord,false);
  writeEndRecord(true);
  currentSectName=label;
  modificationRecord="";
}
if(!isComment&&opcode!="END"){
writeData = "\n********************object program for "+ label+" **************";
  writeToFile(objectFile,writeData);

writeData = "\nH^"+expandString(label,6,' ',true)+'^'+intToHex(address,6);
  writeToFile(objectFile,writeData);
  }

  readIntermediateFile(interFile,isComment,lineNumber,address,blockNumber,label,opcode,operand,comment);
    objectCode = "";
}

int main(){
  fileName = "assembly_code.txt";

  cout<<"\nLoading OPTAB"<<endl;
  load_tables();

  cout<<"\nPerforming Pass 1"<<endl;
  cout<<"Writing the Intermediate File to 'intermediate_"<<fileName<<"'"<<endl;
  cout<<"Writing the Error File to 'error_"<<fileName<<"'"<<endl;
  pass1();

cout<<"Making the Symbol Table"<<endl;
  printtab.open("tables_"+fileName) ;
  writeToFile(printtab,"---------------------------------SYMBOL TABLE---------------------------------\n") ;
    for (auto const& it: SYMTAB) { 
        writestring+=it.first+":-\t"+ "name:"+it.second.name+"\t|"+ "address:"+it.second.address+"\t|"+ "relative:"+intToHex(it.second.relative)+" \n" ;
    } 
    writeToFile(printtab,writestring) ;

writestring="" ;
    cout<<"Making the Literal Table"<<endl;
  
  writeToFile(printtab,"---------------------------------LITERAL TABLE---------------------------------\n") ;
    for (auto const& it: LITTAB) { 
        writestring+=it.first+":-\t"+ "value:"+it.second.value+"\t|"+ "address:"+it.second.address+" \n" ;
    } 
    writeToFile(printtab,writestring) ;
    
writestring="" ;
    cout<<"Making the Block Table"<<endl;
  
  writeToFile(printtab,"---------------------------------BLOCK TABLE---------------------------------\n") ;
    for (auto const& it: BLOCKS) { 
        writestring+=it.first+":-\t"+ "value:"+it.second.name+"\t|"+ "address:"+it.second.startAddress+" \n" ;
    } 
    writeToFile(printtab,writestring) ;
  cout<<"\nPerforming Pass 2"<<endl;
  cout<<"Writing the Object File to 'object_"<<fileName<<"'"<<endl;
  cout<<"Writing the Listing File to 'listing_"<<fileName<<"'"<<endl;
  pass2();

}
