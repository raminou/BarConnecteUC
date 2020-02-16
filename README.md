# BarConnecteUC

BarConnecte (J'ai Soif Connect) Program on ESP8266

This repository is link to the backend: [https://github.com/raminou/BarConnecteWeb](https://github.com/raminou/BarConnecteWeb) and to the application: [https://github.com/raminou/BarConnecteApp](https://github.com/raminou/BarConnecteApp)

## Operation

The program does:
- wait for the glass to be place on the weight scale
- make a request to the backend to get the recipe of the next drink
- make another request to change the drink status
- fill the glass using pumps with time or weight scale to control the quantities
- make a request to indicate that the glass can be take
- make a last request to indicate that the glass has been took

