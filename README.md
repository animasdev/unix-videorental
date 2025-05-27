# Unix Video Rental Portal
Progetto creato per l'esame di Laboratorio di Sistemi Operativi.
## Pre-requisiti
- docker
- docker compose
---
## Istruzioni
Assicurarsi che il deamon di docker sia in esecuzione.
Posizionarsi nella root di progetto e lanciare il seguente comando
```
docker-compose up --build -d
```

Una volta completata con successo la creazione del container, procedere al setup del server e poi a lanciare il client.

---

### Server
In un nuovo terminale eseguire per connettersi al container del server.
```
docker-compose exec server /bin/bash
```
Avviare il server con il seguente comand
```
./rental-server
```
Se nella cartella db del progetto non esiste un database, ne verrà creato uno e partirà il setup interattivo in cui si crea l'utenza di amministrazione.
>**Nota bene**:
>Il database è nella cartella db del progetto, è possibile modificarne il nome nel file .env ma non va modificata la posizione se si esegue il docker-compose.

Quando il server è posto in ascolto sul suo socket il setup è terminato. **NON** chiudere il terminale.

---

### Client
In un nuovo terminale eseugire il seguente comando:
```
docker-compose exec client bash
```
dopo di che si può lanciare l'eseguibile del client
```
./rental-client
```
Se come nome utente viene inserito un utente sconoscito il client chiederà una password e l'utente verrà creato. Si consiglia di entrare come admin la prima volta per inserire qualche articolo.

### In locale
È possibile eseguire sia il server che il client in locale compilandoli con il gcc.
Occorre avere nel proprio sistema le librerie di sviluppo di sqlite3.
I risultati potrebbero variare.