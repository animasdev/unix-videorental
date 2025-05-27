
# Implementazione tecnica
## Struttura generale
il software ha un'architettura client-server, basata su socket locali unix.
Il server ascolta per connessioni su un determinato socket, e per ogni richiesta crea un nuovo processo.\
Il processo figlio mantiene la connessione attiva fino a che il client non si disconnetta;\
il processo padre chiude la connessione con il client e rimane in ascolto per altre connesioni.

## Protocollo di comunicazione 
È stato implementato un protocollo ad hoc per la comunicazione client server, basato concettualmente su scambio di messaggi stateless e strutturato similmente al protocollo HTTP.\
il client manda una richiesta specifica al server e ne attende la risposta.
La richiesta è una stringa composta da:
```
comando sotto-comando [parametri...]
```
i comandi sono:
- CHECK USER per controllare l'esistenza di una risorsa nel sistema
- LOGIN per autenticare un utente
- REGISTER per registrare un utente
- MOVIE per operare su i video del sistema
- RENT per operare sui prestiti del sistema 
- CART per operare sui carrelli

## Gestione Dati
Tutti i dati dell'applicazione sono conservati in un database sqlite3 basato su file.\
Il database comunica solo con il server che gestisce l'accesso ai dati del client attraverso i comandi di cui sopra.
### Utenti
La tabella `Users` raccoglie le seguenti informazioni per ogni utente:
- username
- password
- is_admin, flag che indica se l'utente è un admin

Al momento l'unico admin inserito è quello del setup, non se ne possono aggiungere altri tramite client.

Il comando `CHECK USER <username>` controlla l'esistenza dell'utente nel database e risponde con la prossima azione da intraprendere.\
Se l'utente non esiste, risponde con `REGISTER`\
Se l'utente esiste risponde con `LOGIN`\
Il comando `LOGIN <username> <password>` controlla l'esattezza della password e restituisce l'id e il ruolo (1 se admin, 0 altrimenti)\
il comando `REGISTER <username> <password>` registra l'utente come utente normale, quindi no admin.

Allo stato attuale non è implementato nessun meccanismo di autenticazione: conoscendo il protocollo un client "malevolo" potrebbe impersonare un qualsiasi utente di cui conosca l'id.\
Una soluzione potrebbe essere legare a livello di server il codice utente con il pid del processo client dopo che quest'ultimo si sia autenticato con successo.\
A questo punto il server prima di accettare qualsiasi richiesta che coinvolga gli id utente controlla il pid da cui proviene e se non proviene dal giusto pid la richiesta viene scartata, magari con una risposta ad hoc.

### Film
La tabella `Videos` raccoglie le seguenti informazioni per ogni film:
- titolo,
- copie totali, la totalità delle copie presenti a sistema

i sottocomandi MOVIE sono:
- `MOVIE ADD <title> <nr_copies>` per aggiungere un video, risponde con l'id del film nel db. 
- `MOVIE SEARCH <query>` per ricercare il film per titolo nel db. la ricerca avviene per sottostringa, viene restituita una lista di film.
- `MOVIE GET <id>` restituisce i dati del film identificato dall'id.

Per gestire la disponibilità dei film, si utilizzano query in join con la tabella dei prestiti 'Rentals' in questo modo si contano dinamicamente le copie prese in prestito.

### Prestiti
La tabella `Rentals` raccoglie le seguenti informazioni per ogni prestito:
- username
- id film
- timestamp di inizio prestito
- data di scadenza del prestito
- data di restituzione (se presente)
- flag di reminder

I sottocomandi RENT sono:
- `RENT ADD <user_id> <movie_id>` per aggiungere un prestito, con data di inizio la data attuale e data di fine prestito 7 giorni a partire dalla data attuale. Restituisce tutti i dati del prestito appena aggiunto. Sono previsti messaggi di errore in caso di numero eccessivi di prestiti o prestiti duplicati.
- `RENT SEARCH <username|ALL> <include_returned>` permette di cercare tutti i prestiti di un determinato utente. se si utilizza la query 'ALL' si troveranno tutti i prestiti. Il flag include_returnd (0 o 1) permette di escludere o meno i prestiti che sono stati restituiti
- `RENT PUT <id_rental>` permette di restituire un prestito nella data corrente
- `RENT REMIND <id_rental> <flg_reminder>` permette di settare il flag di reminder.

Il flag di reminder viene utilizzato per mandare avvisi ai client nel caso un prestito di un utente loggato sia 'overdue', data scadenza passata.\
Per ogni prestito di un utente, il suo client chiede al server la presenza del flag e mostra l'avviso a video, dopo di che lo setta a 0.


### Carrello
La tabella `Carts` raccoglie le seguenti informazioni per ogni carrello:
- username
- id film

i sottocomandi CART sono:
- `CART ADD <user_id> <movie_id>` per aggiungere un film al carrello
- `CART DEL <cart_id>` per rimuovere un film dal carrello
- `CART GET <username>` per ottenere tutti gli elementi di un carrello

### Restituzione di liste
Per sincronizzare correttamente client e server, i comandi che prevedono di restituire una lista di oggetti restituiscono al client il numero di oggetti trovati.
Dopo questo messaggio, il server rimane in attesa che il client invii il comando `NEXT` per ottenere il prossimo elemento.