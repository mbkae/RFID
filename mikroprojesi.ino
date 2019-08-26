#include <EEPROM.h>     //Okuma ve yazma işini UID ile EEPROM a kaydetme işlemi.
#include <SPI.h>        //RC522 modülü SPI protokolünü kullanır.
#include <MFRC522.h>    //Mifare RC522 kütüphanesi
#define COMMON_ANODE     
#ifdef COMMON_ANODE
#define yak HIGH
#define sondur LOW
#else
#define yak LOW
#define sondur HIGH
#endif

#define kirmizi 7   // Led pinleri seçildi.
#define yesil 6
#define mavi 5

#define roleLED 4    //Röle pini seçildi.
#define buton 3    //Master kartını silmek için buton pini seçildi.


boolean match=false;   
boolean program=false;  //Programlama modu başlatma.

int basarili;    //Başarılı bir şekilde sayı okuyabilmek için integer atıldı.

byte storedCard[4];   //Kart EEPROM tarafından okundu.
byte readCard[4];     //RFID modül ile ID tarandı.
byte masterCard[4];   //Master kart ID'si EEPROM'a aktarıldı.

#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN,RST_PIN);  //MFRC522 örneği oluşturuldu.


///////////////////////YÜKLEME/////////////////////

void setup()
{
  //Arduino Pin Konfigrasyonu
  pinMode(kirmizi,OUTPUT);
  pinMode(yesil,OUTPUT);
  pinMode(mavi,OUTPUT);
  pinMode(buton,INPUT_PULLUP);
  pinMode(roleLED,OUTPUT);
 
  
  digitalWrite(roleLED,HIGH);        
  digitalWrite(kirmizi,sondur);    
  digitalWrite(yesil,sondur);  
  digitalWrite(mavi,sondur);   

  //Protokol konfigrasyonu
  Serial.begin(9600);    //PC ile seri iletim başlat.
  SPI.begin();           //MFRC522 donanımı SPI protokolünü kullanır.
  mfrc522.PCD_Init();    //MFRC522 donanımını başlat.

  Serial.println(F("Mikrodenetleyiciler Dersi Projesi"));  //Hata ayıklama amacıyla
  Serial.println(F("Muhammed Kurtuluş-Ragıp Temirtaş-Mehmet Burak İmren"));
  ShowReaderDetails();   //PCD ve MFRC522 kart okuyucu bilgisi.

  //Silme kodu butona basıldığında EEPROM içindeki bilgileri siler.
  if(digitalRead(buton)==LOW)
  {
    
    digitalWrite(kirmizi,yak); //Kırmızı led sildiğimizi bilgilendirmek amacıyla yanık kalır.
        
    delay(500);                       
    if (digitalRead(buton)==LOW)       
    {
      Serial.println(F("EEPROM silinmeye baslaniyor."));
      for (int x=0; x<EEPROM.length();x=x+1)  //EEPROM adresinin döngü sonu
      {
        if(EEPROM.read(x)==0)   //EEPROM adresi sıfır olursa
        {
          
        }
        else
        {
          EEPROM.write(x,0);
        }
       }
       Serial.println(F("EEPROM Basariyla Silindi.."));
       digitalWrite(kirmizi,sondur);
       delay(200);
       digitalWrite(kirmizi,yak);
       delay(200);
       digitalWrite(kirmizi,sondur);
       delay(200);
       digitalWrite(kirmizi,yak);
       delay(200);
       digitalWrite(kirmizi,sondur);
       }

       else
       {
        Serial.println(F("Silme islemi iptal edildi."));
        digitalWrite(kirmizi,sondur);
       }
      }

/*Master Kart tanımlı mı kontrol edilecek.Eğer kullanıcı Master Kart seçmediyse      
  yeni bir Master Kart EEPROM a kaydedilecek.EEPROM kayıtlarının haricinde 143 EEPROM adresi tutulabilir.
  EEPROM adresi sayısı 143.
 */

 if (EEPROM.read(1) != 143)
 {
  
  Serial.println(F("Master Kart Secilmedi."));
  Serial.println(F("Master Karti secmek icin kartinizi okutunuz.."));
  do
  {
    basarili=getID();             //basarili 1 olduğu zaman okuyucu düzenlenir aksi halde 0 olucaktır.
    digitalWrite(mavi,yak);    //Master Kartın kaydedilmesi için gösterildiğini ifade eder.
    delay(200);
    digitalWrite(mavi,sondur);
    delay(200);
  }
  while(!basarili);              //Başarılı bir şekilde okuyamadıysa Başarılı okuma işlemini yapmicaktır.
  for (int j=0; j<4; j++)           //4 kez döngü
  {
    EEPROM.write(2+j,readCard[j]);    //UID EPPROM a yazıldı, 3. adres başla.
  }
  EEPROM.write(1,143);      //EEPROM a Master Kartı kaydettik.
  Serial.println(F("Master Kart kaydedildi.."));
  }

Serial.println(F("**************************"));
Serial.println(F("Master Kart UID:"));
for(int i=0; i<4; i++)                    //EEPROM da Master Kartın UID'si okundu.
{                                         //Master Kart yazıldı.
  masterCard[i]=EEPROM.read(2+i);
  Serial.print(masterCard[i],HEX);
}

Serial.println("");
Serial.println(F("**************************"));
Serial.println(F("Hersey Hazir!!"));
Serial.println(F("Kart okutulmasi icin bekleniyor..."));
cycleLeds();                                         //Herşeyin hazır olduğunu kullanıcıya haber vermek için geri bildirim.
}


void loop()
{
  do
  {
    basarili=getID();       //basarili 1 olursa kartı oku aksi halde 0
    if(program)
    {
      cycleLeds();              //Program modu geri bildirimi yeni kart okutulmasını bekliyor.
    }
    else
    {
      normalModeOn();        //Normal mod, mavi led açık diğer tümü kapalı.
    }
  }
  while(!basarili);   
  if(program)
  {
    if(isMaster(readCard))    //Master Kart tekrar okutulursa programdan çıkar.
    {
      Serial.println(F("Master Kart Okundu.."));
      Serial.println(F("Programdan cikis yapiliyor.."));
      Serial.println(F("**************************"));
      program=false;
      return;
    }
    else
    {
      if(findID(readCard))   //Okunan kart silinmek isteniyorsa
      {
        Serial.println(F("Okunan kart siliniyor.."));
        deleteID(readCard);
        Serial.println(F("**************************"));
      }
      else                   //Okunan kart kaydedilmek isteniyorsa
      {
        Serial.println(F("Okunan kart hafizaya kaydediliyor.."));
        writeID(readCard);
        Serial.println(F("**************************"));
      }
    }
  }
  else
  {
    if(isMaster(readCard))   //Master Kart okunursa programa giriş yapılıyor.
    {
      program=true;
      Serial.println(F("Merhaba, programa giris yapiliyor."));
      int count=EEPROM.read(0);
      Serial.print(F("Kayıtli kullanici "));
      Serial.print(count);
      Serial.print(F(" sayisi kadardir."));
      Serial.println("");
      Serial.println(F("Eklemek veya cikarmak istediginiz karti okutunuz."));
      Serial.println(F("*****************************"));
    }
    else
    {
      if(findID(readCard))
      {
        Serial.println(F("Hosgeldin, gecis izni verildi."));
        Serial.println(F("**************************"));
        izinverildi(1000);     //1 saniyede kilitli kapıyı aç.
      }
      else
      {
        Serial.println(F("Gecis izni verilmedi."));
        Serial.println(F("**************************"));
        izinyok();
      }
    }
  }
}

////////////////////ERİŞİM İZNİ//////////////////////

void izinverildi(int setDelay)
{
  digitalWrite(mavi,sondur);  //mavi led sönük
  digitalWrite(kirmizi,sondur);   //kırmızı led sönük
  digitalWrite(yesil,yak);  //yeşil led yanık
  digitalWrite(roleLED,LOW);        //kapı kilidi açıldı
  delay(setDelay);                //kapının açılması için zaman kazanıyoruz.
  digitalWrite(roleLED,HIGH);       //kapı kilitlendi
  delay(750);                    //1 saniye sonra yeşil led de sönücek
}

////////////////ERİŞİM İZNİ VERİLMEDİ/////////////////////

void izinyok()
{
  digitalWrite(yesil,sondur);   
  digitalWrite(mavi,sondur);    
  digitalWrite(kirmizi,yak);      
  delay(1000);
}

/////////////////////KART OKUYUCU UID AYARLAMA////////////////////////////

int getID()
{
  //Kart okuyucuyu hazır ediyoruz
  if(!mfrc522.PICC_IsNewCardPresent())   //yeni bir kart okutup devam ediyoruz.
  {
    return 0;
  }
  if(!mfrc522.PICC_ReadCardSerial())     //kartın serial numarasını alıp devam ediyoruz.
  {
    return 0;
  }
  //4 ve 7 byte UID'ler mevcut biz 4 byte olanı kullandık.
  Serial.println(F("Kartin UID'sini taratin..."));
  for (int i=0; i<4; i++)
  {
    readCard[i]=mfrc522.uid.uidByte[i];
    Serial.print(readCard[i],HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); //Okuma durduruluyor.
  return 1;
}

void ShowReaderDetails() {
  // MFRC522 bilgileri alınması ve bağlantı kontrolü.
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("DİKKAT! Haberlesme yetmezligi, MFRC522'yi dogru bagladiginizdan emin olun."));
    while(true);  
  }
}


//////////////////Cycle Leds(Program Modu)///////////////////////

void cycleLeds() {
  digitalWrite(kirmizi, sondur);   
  digitalWrite(yesil, yak);   
  digitalWrite(mavi, sondur);   
  delay(200);
  digitalWrite(kirmizi, sondur);  
  digitalWrite(yesil, sondur);  
  digitalWrite(mavi, yak);  
  delay(200);
  digitalWrite(kirmizi, yak);   
  digitalWrite(yesil, sondur);  
  digitalWrite(mavi, sondur);   
  delay(200);
}


/////////////////Normal Led Modu//////////////////////////////

void normalModeOn () {
  digitalWrite(mavi, yak);   // mavi led yanık ve program kart okumaya hazır
  digitalWrite(kirmizi, sondur);  
  digitalWrite(yesil, sondur);  
  digitalWrite(roleLED, HIGH);    
}

///////////////////////EEPROM için ID Okuma////////////////////////

void readID( int number )
{
  int start=(number*4)+2;    //başlama pozisyonu
  for (int i=0; i<4; i++)     //4 byte alamabilmek için 4 kez döngü kurduk.
  {
    storedCard[i]=EEPROM.read(start+i); //  EEPROM dan diziye okunabilen değerler atadık.
  }
}

////////////////////EEPROM a ID Ekleme///////////////////////////////////

void writeID(byte a[])
{
  if (!findID(a))    
  {
    int num=EEPROM.read(0);  
    int start= (num*4)+6;
    num++;
    EEPROM.write(0,num);
    for(int j=0;j<4;j++)
    {
      EEPROM.write(start+j,a[j]);
    }
    successWrite();
    Serial.println(F("Basarili bir sekilde ID kaydi EEPROM'a eklendi.."));
    }
    else
    {
      failedWrite();
      Serial.println(F("izinyok! Yanlis ID veya kullanici sinirina ulasildi"));
    }
}


/////////////////////////EEPROM'dan ID Silme////////////////////////

void deleteID(byte a[])
{
  if (!findID(a))       //Önceden EEPROM'dan silinen karta sahipmiyiz kontrol et.
  {
    failedWrite();             //değilse
   Serial.println(F("izinyok! Yanlis ID veya kullanici sinirina ulasildi"));
  }
  else
  {
    int num=EEPROM.read(0);
    int slot;
    int start;
    int looping;
    int j;
    int count=EEPROM.read(0);  //Kart numarasını saklayan ilk EEPROM'un ilk byte'ını oku
    slot=findIDSLOT(a);
    start=(slot*4)+2;
    looping=((num-slot)*4);
    num--;                            //tek sayacı azaltma
    EEPROM.write(0,num);
    for(j=0;j<looping;j++)
    {
      EEPROM.write(start+j,EEPROM.read(start+4+j));
    }
    for(int k=0;k<4;k++)
    {
      EEPROM.write(start+j+k,0);
    }
    successDelete();
    Serial.println(F("Basarili bir sekilde ID kaydi EEPROM'dan silinmistir.."));
  }
}


///////////////Byte'ların Kontrolü///////////////////

boolean checkTwo (byte a[],byte b[])
{
  if (a[0] != NULL)
  match=true;
  for (int k=0; k<4; k++)
  {
    if(a[k] != b[k])
    {
      match=false;
    }
    if(match)
    {
      return true;
    }
    else 
    {
      return false;
    }
  }
}

//////////////////////Boşluk Bulma////////////////////////////

int findIDSLOT (byte find[] )
{
  int count = EEPROM.read(0);     //EEPROM ile ilk byte ı okuyacağız.
  for(int i=1; i<=count; i++)      //Döngüdeki her EEPROM girişi için
  {
    readID(i);                  //EEPROM daki ID yi okuyacak ve Storedcard[4] de saklayacağız.
    if (checkTwo(find, storedCard))  // Saklı Kartlar da olup olmadığının kontrolü.
    { //aynı ID'e sahip kart bulursa geçişe izin vericek.
      return i;   //kartın slot numarası
      break;       //aramayı durduracak.
    }
  }
}

///////////////////////EEPROM'da ID Bulma//////////////////////

boolean findID (byte find[] )
{
  int count = EEPROM.read(0);    //EEPROM'daki ilk byte'ı oku
  for(int i=1; i <= count; i++)    //Önceden giriş yapılmış mı kontrolü.
  {
    readID(i);      
    if(checkTwo(find,storedCard) )
    {
      return true;
      break;
    }
    else
    {
      //değilse return false
    }
  }
  return false;
}


///////////////////Başarılı Şekilde EEPROM'a Yazma//////////////////

void successWrite() 
{
  digitalWrite(mavi, sondur);   
  digitalWrite(kirmizi, sondur);  
  digitalWrite(yesil, sondur);  
  delay(200);
  digitalWrite(yesil, yak);   
  delay(200);
  digitalWrite(yesil, sondur);  
  delay(200);
  digitalWrite(yesil, yak);   
  delay(200);
  digitalWrite(yesil, sondur);  
  delay(200);
  digitalWrite(yesil, yak);   
  delay(200);
}

/////////////////////EEPROM'a Yazma İşlemi Başarısız/////////////////

void failedWrite() 
{
  digitalWrite(mavi, sondur);   
  digitalWrite(kirmizi, sondur);  
  digitalWrite(yesil, sondur);  
  delay(200);
  digitalWrite(kirmizi, yak);   
  delay(200);
  digitalWrite(kirmizi, sondur);  
  delay(200);
  digitalWrite(kirmizi, yak);   
  delay(200);
  digitalWrite(kirmizi, sondur);  
  delay(200);
  digitalWrite(kirmizi, yak);   
  delay(200);
}

////////////////////Silme İşlemi Başarılı///////////////////////////

void successDelete() 
{
  digitalWrite(mavi, sondur);   
  digitalWrite(kirmizi, sondur);  
  digitalWrite(yesil, sondur);  
  delay(200);
  digitalWrite(mavi, yak);  
  delay(200);
  digitalWrite(mavi, sondur);   
  delay(200);
  digitalWrite(mavi, yak);  
  delay(200);
  digitalWrite(mavi, sondur);   
  delay(200);
  digitalWrite(mavi, yak);  
  delay(200);
}

////////////////Master Kartın Doğruluğunun Tespiti///////////////////

boolean isMaster (byte test[])
{
  if(checkTwo(test,masterCard))
  return true;
  else
  return false;
  
}
