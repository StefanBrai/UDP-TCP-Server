README - Tema2 Pcom .-. Aplicatie Client-Server pentru gestionarea mesajelor.

-> Tema urmareste implementarea a doua entitati , server-ul si clientul.

SERVER (+)     Serverul reprezinta un punct de receptie a mesajelor venite pe o cale udp de la clienti udp implementati de catre echipa responsabila de tema 
            si un punct de comunicare multipla cu diversi abonati tcp care comunica cu serverul prin socket-uri tcp , acestia fiind nevoiti sa trimita un
            request pe socket-ul tcp principal al server-ului , urmand sa isi trimita ID-ul de identificare pe socketul lor dezignat. Serverul functioneaza
            pe urmatorul principiu : Programul ruleaza in bucla infinita pana cand intalneste o eroare de la functiile de retea sau pana i se parseaza comanda
            de "exit" de la tastatura . In interiorul buclei while , programul itereaza prin lista de file descriptori atasati acestuia , asigurandu-se ca fiecare
            socket/fd este valid . Sunt 4 posibilitati pentru un fd : 

                                - stdin : de unde serverul preia comenzi de la tastatura.
                                
                                - tcp_socket : socket de unde preia conexiuni tcp de la potentiali clienti ; in cadrul if-case -ului asociat acestui socket se analizeaza si ID-ul primit
                                de la acest potential client , pentru a decide daca e un client nou , recurent , sau daca incearca sa se logheze cu ID-ul unui client care e deja conectat(doi
                                clienti nu pot avea acelasi ID);

                                - udp_socket : socket de unde preia mesaje despre diverse topicuri ; in cadrul if-case -ului asociat acestui socket se proceseaza mesajul primit
                                si se proceseaza in conformitate cu standardele pentru structura mesajului pe care serverul trebuie sa il trimita clientilor . Tot in acest moment,
                                serverul trimite mesajul primit , legat de un topic , abonatilor corespunzatori.

                                -un socket al unui client : socket unde se preiau comenzi de subscribe sau unsubscribe de la un topic , si pe care se trimit mesaje legate de topicurile
                                la care clientul asociat acelui socket este abonat.

                Structurile folosite in cadrul manipularii datelor sunt: 
                                                                        upd_rcv_msg : unde se stocheaza un mesaj udp primit de la un client udp 
                                                                         
                                                                        tcp_server_msg : o structura menita sa contina procesarea mesajului udp si inca doua caracteristici , portul si ip-ul 
                                                                                        de pe care a venit mesajul
                                                                        
                                                                        tcp_client_msg : o structura ce contine comanda de sub/unsub a unui client + topicul pe care doreste sa se execute comanda

                                                                        client_database : o structura ce stocheaza date despre un client , si anume ID-ul sau , socket-ul de pe care a venit ID-ul si
                                                                                        topicurile la care acesta este abonat; atunci cand un client se deconecteaza , el nu este sters din lista de clienti,
                                                                                        ci doar i se asociaza un socket invalid prin utilizarea unui macro numit EMPTY_ID_SOCKET_LINK

CLIENT(+)       Clientul este cel care doreste sa se conecteze la server , iar dupa ce acesta stabileste legatura , trimite cereri de subscribe/unsubscribe si primeste date  despre topicurile la care s-a abonat atunci cand acestea
            sunt actualizate. Actiunea se intampla tot intr-o bucla infinita , unde se verifica stdin-ul pentru comenzi si socketul legat la server pentru mesaje legate de topicuri .
                Structurile folosite sunt aceleasi ca la server , minus udp_recv_msg si client_database.
                Serverul trimite mesaje cientulu de tip tcp_server_msg , iar clientul ii trimite mesaje serverului de tip tcp_client_msg.


**Implementarea wildcardurilor a fost incercata , dar nu trece testele.
**Tema a fost realizata si folosind cunostiintele dobandite in laboratoarele 5 si 7
                                                                                                        
