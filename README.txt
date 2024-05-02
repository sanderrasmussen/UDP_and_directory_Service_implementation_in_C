Til sensor:
Jeg kommer i denne readme.txt filen til å fokusere mest på de tingene som jeg slet mest med og som jeg trengte å analysere mest.

d1_udp.c :

    Det første jeg gjør er å create client. Jeg oppretter client og initialiserer seqno til 0. Disse går derretter inn iD1_client structen. Clienten kan slettes med d1_delete metoden som frigjør pekerene slik at vi ikke får minnelekasjer. 

    Metoden d1_get_peer_info gjør oppslag for å finne IP-adressen til en client og fyller en D1Peer struct med denne informasjonen. Den støtter IPv4 og UDP-kommunikasjon. Vi leter etter adresse som kan brukes og om vi finner så setter vi adressen i D1Peer.

--recv_data() og checksum
    I d1_recv_data metoden tar jeg lagrer jeg mottatt data fra serveren i et buffer og caster den til D1Header i en header variablen slik at det blir lettere å lese header dataen. Deretter lagrer jeg headeren sin checksum omgjort til host order i en annen variabel og setter checksummen til 0. Etter det gir jeg headeren og bufferen uten header til en funksjon som beregner checksummen.
    Checksum metoden fungerer slik at jeg har to variabler som jeg xorer med bufferet annen hver tur i loopingen igjennom bufferet. Til slutt etter jeg har loopet igjennom headeren og bufferen setter jeg de første 8 bitene til den første variablen "even" og de siste 8 retur bitene til "odd".
    Jeg brukte en del tid på å skrive denne metoden da jeg syntes det var vanskelig å forstå hvordan checksummen skulle beregnes med tanke på rekkefølge av xoren. Jeg endte opp med med å rekonstruere bufferen fra header og message buffer til en samlet packe slik som mottat av serveren igjen.
    Deretter i recv_data metoden så converterer jeg header verdiene til host order og sjekker deretter om den beregnede checksummen er lik checksummen som var i headeren jeg mottok fra serveren.
    Dersom checksummen og størrelsen på pakken er rett så kaller jeg send_ack med header->flags &SEQNO som ackno, altså jeg sender samme ack som seqno serveren sendte i flaggene på headeren.
    Dersom checksummen eller størrelsen er feil sendes feil ack nummer til serveren slik at den kan sende meg pakken på nytt.

    Send ack metoden min lager en header hvor flaggene settes til FLAG_ACK. Her møtte jeg på et problem. Måten jeg setter ack nummeret på er ved å sjekke om seqno argumentet gitt til funksjonen er større enn 0. 
    Jeg fikk noen ganger problemet at seqno ble 128 istedenfor 1. Jeg har derimot altid fått binær resultat altså enten 0 og 1, eller 0 og 128. 
    Derfor har det ikke vært noe problem i testene mine ved å sette ACKNO flagget dersom seqno er større enn 0. Denne koden har passert alle testene, men jeg er usikker på hvorfor seqno noen ganger blir 128, kansje overflow eller problem ved type casting?
    Videre beregnes checksum og verdiene converteres til network order før ack headeren blir sendt.

    Min Wait_ack metode allokerer buffer og venter på pakke som lagres i bufferet. Deretter konverteres verdiene til host order.
    Jeg sjekker om pakken er en ack pakke, altså om ack flagget er satt. Om dette er tilfellet sjekker jeg om seqno i pakken er lik peer->next_seqno og flipper next_seqno fra 0 til 1 eller fra 1 til 0.

    I d1_send_data 

    NOTATER TIL SELV SJEKK OM RETUR ER RETT ALTSÅ 1 ELLER -1, 
    LEGG TIL IF MALLOC==NULL
            PERROR MALLOC NULL
    OSV.