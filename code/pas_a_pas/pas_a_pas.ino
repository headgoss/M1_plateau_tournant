/*
 * Code arduino pour le controle d'un plateau tournant par bluetooth
 * Réalisé par GROSSARD Mathieu et SCHULTZ Quentin
 */
 
#include "BluetoothSerial.h"
#include <CheapStepper.h>
#include <String>

// next, declare the stepper
#define IN1 19
#define IN2 18
#define IN3 5
#define IN4 17
//pin bouton
#define BP 2

CheapStepper stepper (IN1,IN2,IN3,IN4);  

BluetoothSerial SerialBT;

//Déclaration des variables lors de l'initialisation de l'esp

bool moveClockwise = true;
unsigned long moveStartTime = 0; // this will save the time (millis()) when we started each new move
String message=" ";
String info = " ";
String type = "Manual";
String mod = "Disconnect";
String a = "a";
int stepsLeft;
int cpt = 3;
volatile bool bouton = false;
long deboucing_time = 20;
volatile unsigned long last_micros;


/****************************************************************************
 * INTERRUPT ROUTINES avec anti rebond pour le bouton poussoir
 ****************************************************************************/
void BP_Function(){
  if((long)(micros()-last_micros) >= deboucing_time*1000)
  {
    bouton = true;
    last_micros = micros();
  }
}


void setup() {

  // let's run the stepper at 5rpm

  stepper.setRpm(5);

  //Communication
  SerialBT.begin("ESP32");
  Serial.begin(115200);

  //Bouton poussoir déclaration
  attachInterrupt(digitalPinToInterrupt(BP), BP_Function, FALLING);

  stepper.newMoveTo(moveClockwise, 2048);
  moveStartTime = millis(); // let's save the time at which we started this move
  
}

void loop() {
  stepper.run();
  if (SerialBT.available()) // réception d'un message
  {
    stepper.stop();
    message = SerialBT.readString(); // lecture du message reçu    
    message.trim();
    Serial.println(message);
  }
  if ( message == "Reverse")
  {
    //Change de sens
    message = " ";
    moveClockwise = !moveClockwise;  
  } 
  else if (message == "Auto")
  {
    //Le mode type
    message = " ";
    type = "Auto";
  }
  else if (message == "Manual")
  {
    //Le mode type
    message = " ";
    type = "Manual";
  }
  else if (message == "Connect")
  {
    //Si une connection Bluetooth avec L'application
    message = " ";
    mod = "Connect";
  }
  else if (message == "Disconnect")
  {
    //Si on se déconnecte de l'application
    message = " ";
    mod = "Disconnect";
  }
  //Partie si on est connecté
  if (mod=="Connect")
  {    
    if (type == "Auto")
      { //On vérifie si on a reçu un angle de rotation      
        if (message.startsWith(a)==true)
        {      
          message.remove(0,1);
          if (message.startsWith("-")==true)
          {
            moveClockwise = !moveClockwise;  
            message.remove(0,1);      
          }
          info = message;
          Serial.println(info);
          message = " ";      
        }
        //Si on à un angle de rotation on tourne de tel angle puis photo
        if (info!=" ")
        {
            stepper.moveDegrees(moveClockwise, info.toInt());//mouvement bloquant
            //pause de 5s
            delay(2000);
            //Envoie du message pour prise de photo
            SerialBT.println("p");
            delay(3000);
        }          
      }
      
      else if (type == "Manual")
      { //On vérifie si on a reçu un angle de rotation 
        if (message.startsWith(a)==true)
        {      
          message.remove(0,1);
          if (message.startsWith("-")==true)
          {
            moveClockwise = !moveClockwise;  
            message.remove(0,1);
            info = message;
            Serial.println(info);
            message = " ";
            //Si on à un angle de rotation on tourne de tel angle puis photo
            stepper.moveDegrees(moveClockwise, info.toInt());//mouvement bloquant
            
            delay(2000);
            SerialBT.println("p");
            delay(3000);
          }
          else 
          {
            info = message;
            Serial.println(info);
            message = " ";
            //Si on à un angle de rotation on tourne de tel angle puis photo
            stepper.moveDegrees(moveClockwise, info.toInt());//mouvement bloquant
            
            delay(2000);
            SerialBT.println("p");
            delay(3000);
            }     
          }
      }
      //On règle selon la vitesse reçu  
      if (message!=" ")
      {
        Serial.print("Vitesse :");
        if (message.length()!= 1)
          {
            message.remove(0,message.length()-1);
          }
        Serial.println(message);
        stepper.setRpm(message.toInt());
      }
  }
  //Si pas connecté la vitesse se modifie avec le bouton
  if (mod == "Disconnect")
  {
    if(bouton == true)
    {
      cpt+=1;
      stepper.stop();
      if (cpt>3)
        {
          cpt = 1;
        }
        //Les diférentes vitesses
      switch(cpt){
        case 1:
          stepper.setRpm(2);
          break;
        case 2:
          stepper.setRpm(10);
          break;
        case 3:
          stepper.setRpm(5);
          break;
      }
      bouton = false; 
    }
  }
  
  // let's check how many steps are left in the current move:  
  stepsLeft = stepper.getStepsLeft();
  
  if (stepsLeft == 0)
  {
  // if the current move is done...
      unsigned long timeTook = millis() - moveStartTime; // calculate time elapsed since move start   
      stepper.newMove (moveClockwise, 2048);
      moveStartTime = millis(); // reset move start time
  }
  message = " ";
}
