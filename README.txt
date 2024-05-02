Til sensor:
Jeg kommer i denne readme.txt filen til å fokusere mest på de tingene som jeg slet mest med og som jeg trengte å analysere mest.

d1_udp.c :

--D1_create_client og d1_delete
    Det første jeg gjør er å create client. Jeg oppretter client og initialiserer seqno til 0. Disse går derretter inn i D1_client structen. Clienten kan slettes med d1_delete metoden som frigjør pekerene slik at vi ikke får minnelekasjer. 

--D1_get_peer_info
    Metoden d1_get_peer_info gjør oppslag for å finne IP-adressen til en client og fyller en D1Peer struct med denne informasjonen. Den støtter IPv4 og UDP kommunikasjon. Vi leter etter adresse som kan brukes og om vi finner så setter vi adressen i D1Peer.
    Når getaddrinfo kalles med peername, så bestemmer den om peername er en streng som representerer vertsnavnet eller en desimal IP adresse. Deretter returnerer den en liste over alle relevante adresseoppføringer for det gitte vertsnavnet eller ip adressen.

--Recv_data og calculate_checksum
    I d1_recv_data metoden tar jeg lagrer jeg mottatt data fra serveren i et buffer og caster den til D1Header i en header variablen slik at det blir lettere å lese header dataen. Deretter lagrer jeg headeren sin checksum omgjort til host order i en annen variabel og setter checksummen til 0. Etter det gir jeg headeren og bufferen uten header til en funksjon som beregner checksummen.
    Checksum metoden fungerer slik at jeg har to variabler som jeg xorer med bufferet annen hver tur i loopingen igjennom bufferet. Til slutt etter jeg har loopet igjennom headeren og bufferen setter jeg de første 8 bitene til den første variablen "even" og de siste 8 retur bitene til "odd".
    Jeg brukte en del tid på å skrive denne metoden da jeg syntes det var vanskelig å forstå hvordan checksummen skulle beregnes med tanke på rekkefølge av xoren. Jeg endte opp med med å rekonstruere bufferen fra header og message buffer til en samlet packe slik som mottat av serveren igjen.
    Deretter i recv_data metoden så converterer jeg header verdiene til host order og sjekker deretter om den beregnede checksummen er lik checksummen som var i headeren jeg mottok fra serveren.
    Dersom checksummen og størrelsen på pakken er rett så kaller jeg send_ack med header->flags &SEQNO som ackno, altså jeg sender samme ack som seqno serveren sendte i flaggene på headeren.
    Dersom checksummen eller størrelsen er feil sendes feil ack nummer til serveren slik at den kan sende meg pakken på nytt.

--Send_ack 
    Send ack metoden min lager en header hvor flaggene settes til FLAG_ACK. Her møtte jeg på et problem. Måten jeg setter ack nummeret på er ved å sjekke om seqno argumentet gitt til funksjonen er større enn 0. 
    Jeg fikk noen ganger problemet at seqno ble 128 istedenfor 1. Jeg har derimot altid fått binær resultat altså enten 0 og 1, eller 0 og 128. 
    Derfor har det ikke vært noe problem i testene mine ved å sette ACKNO flagget dersom seqno er større enn 0. Denne koden har passert alle testene, men jeg er usikker på hvorfor seqno noen ganger blir 128, kansje overflow eller problem ved type casting?
    Videre beregnes checksum og verdiene converteres til network order før ack headeren blir sendt.

--Wait_ack
    Min Wait_ack metode allokerer buffer og venter på pakke som lagres i bufferet. Deretter konverteres verdiene til host order.
    Jeg sjekker om pakken er en ack pakke, altså om ack flagget er satt. Om dette er tilfellet sjekker jeg om seqno i pakken er lik peer->next_seqno og flipper next_seqno fra 0 til 1 eller fra 1 til 0.

--Send_data 
    Send data funksjonen sjekker først om buffer er større en max packet size. 
    Deretter callocer jeg plass til header og sjekker at den ikke er NULL.
    Jeg setter datapacket flag og SEQNO flag derson peer->nexseqno >0.
    Feltene settes til network order og checksum beregnes og settes i network order.
    Deretter sendes data pakken til serveren og Wait_ack funksjonen.
    
--Ekstra kommentarer
    Jeg frigjør selfølgelig alle pointers slik at jeg ikke får memory leaks. Samt sjekker om pointere er null og om de er, frigjør de og returnerer feil.

d2_lookup.c :

--D2_client_create
    Jeg kaller D1_create_client og D1_get_peer_info. Deretter settes client->peer til d1peer.

--D2_client_delete
    Her kaller bare d1_delete og frigjør peker til client dersom client != NULL.

--D2_send_request
    Lager request buffer av structen PacketRequest og setter type = TYPE_REQUEST.
    Deretter setter jeg type og id feltene til network order.
    Kaller deretter på bunnlagsfunksjonen d1_send_data, for å sende dataen.

--D2_recv_response_size
    Kaller d1_recv_data og lagrer responsen i et buffer som jeg senere caster til PacketResponseSize peker.
    Setter deretter size feltet i responsen til host order og returnerer dette.

--D2_recv_response
    Kaller d1_recv_data og caster databufferet til PacketResponse.
    Konverter deretter payload størrelsen til host order.
    Siden NetNode ikke har fast størrelse må jeg lese byte for byte. 
    Jeg caster bufferet med data til (uint32_t *) og starter på posisjonen i bufferett etter packetResponse hvor selve payload dataene starter.
    Deretter looper jeg igjennom 4 bytes om gangen altså per uint32_t verdi og flipper den til host order.
    Jeg lager node structer når jeg legger de til i treet i den senere metoden.

--D2_alloc_local_tree
    Her allocerer jeg plass til treet og setter number of nodes feltet i tre structen til argumentet funksjonen tar inn som parameter.
    Deretter allocerer jeg sizeof(NetNode)*num_nodes plasser i nodefeltet til treet.

--D2_free_local_tree
    Her gjør jeg en rask sjekk at pekeren til treet ikke er NULL of frigjør treets pekere til nodene og treets peker.

--D2_add_to_local_tree
    Dette var kansje den mest kompliserte metoden i filen. Sine NetNode var "abriviatet" som jeg tolket som at den ikke sendte alle feltene til struct NetNode så måtte jeg gjøre som i recv_respons metoden.
    Jeg castet bufferet til uint32_t verdier. Deretter loopet jeg igjennom alle og satte 4 bytes om gangen til hvert av feltene i NetNode structen. 
    Når alle verdiene i noden er lest settes den inn i treet i nodelisten på posisjonen node_idx, deretter økes denne verdien med 1.

    Her har jeg litt rar kode : while ( i< num_32_vals -2). Jeg hvet ikke hvorfor jeg må sette -2 bak num_32_vals, men det funker kun om jeg gjør det. 
    Jeg har kjørt mange tester inkludert testen hvor jeg spør serveren etter id 1007 og det virker. Jeg har kjørt flere verdier og koden har ikke feilet noen av gangene for meg. 
    Jeg vet altså ikke hvorfor jeg trenger -2 , men det virker.

    Serveren gir meg nodene med id i stigende rekkefølge, altså id = 0, 1, 2, 3 osv...
    Derfor kan jeg bruke node_idx som den faktiske id til noden og sette den inn i treet på posisjon = node_idx = nodens id. 
    Dermed blir det lett å skrive ut nodene da arrayet med nodene blir et slags map.


 --D2_print_tree og printNodeRecursive
    Her bruker jeg recursjon til å printe ut nodene. Siden alle noder er barn av rotnoden med id = 0 så kaller jeg printNodeRecursive på rotnoden og printer rekursivt ut alle barna.
    På denne måten printes hver node kun ut en gang.
    Jeg har en nestedNr parameter som jeg øker med 1 hver gang rekursjonen går ett lag dypere.
    Siden alle nodene lagres på posisjonen i arrayet som er lik den faktiske id til noden så kan jeg får hvert barn av noden printe ut node id = node.child_id[i] som blir posisjonen i arrayet. 
    For hvert nestedNr printes det ut "--" før nodens info og \n til slutten etter noden, på denne måten trenger jeg ikke lage en streng med fast lengde som kun kan representere en viss mengde barn.
    
