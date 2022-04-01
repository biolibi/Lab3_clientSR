
#include <iostream>
#include <string>   
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <vector>
#include <filesystem>
#include <fstream>

#pragma comment(lib,"ws2_32")
//Source de l'information prise sur la documentation de microsoft en lien avec windows socket 2 : https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-start-page-2
//afin de me permettre de comprendre et faire fonctionner les sockets pour permettre la communication
//entre le serveur et client
using namespace std;


int verification() //methode pour verifier si la prochaine entré est un int
{
    int unNombre;

    while (!(std::cin >> unNombre))
    {
        std::cin.clear();
        std::cin.ignore(1000, '\n');
        std::cout << "ce nombre n'est pas un int valide (ecrivez un nombre valide)\n";
    }
    return unNombre;
}

int main(int argc, char** argv)
{
  
    WSADATA wsaData;
    SOCKET connectionS;
    struct addrinfo *ptr = NULL, hints, *pResult = NULL;

    char sendBuffer[512];
    char recvBuffer[512];
    char* bufferPasFixe;
    vector<string> vecteurPath;

    // lorsqu'on lance le programme 4 arguments sont pris en compte

    if (argc != 4) {

        cout << "ajouter l'adresse du serveur, le nom, le mot de passe d'identification\n";
        return 1;
    }

    // intialise winSock
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
    {
        cout << "erreur lors de l'initiation de WinsockDLL\n";
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // cherche l'information du serveur
    if (getaddrinfo(argv[1],"27015",&hints,&pResult) != 0)
    {
        cout << "erreur lors de la recuperation d'info du client\n";
        //termine le process WinsockDLL
        WSACleanup();
    }

    SOCKET connectSocket = INVALID_SOCKET;
    //permet de définir la limite maximum pour l'envoie de fichier (dans ce cas-ci, 1gb, la limite maximum)

    int limitTailleFichier = 1048576;
    setsockopt(connectSocket, SOL_SOCKET, SO_SNDBUF, to_string(limitTailleFichier).c_str(), 1048576);

       //creation socket
    for (ptr = pResult; ptr!=NULL; ptr = ptr->ai_next)
    {
        connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        if (connectSocket == INVALID_SOCKET)
        {
            cout << "Erreur socket invalide!\n";
            //termine le process WinsockDLL
            WSACleanup();
        }

        // connection serveur
        if (connect(connectSocket, ptr->ai_addr, ptr->ai_addrlen) == SOCKET_ERROR)
        {
            cout << "Erreur connection serveur!\n";
            //ferme le socket et remet la value initial
            closesocket(connectSocket);
            connectSocket = INVALID_SOCKET;
        }
    }

    // libère les ressources utiliser par getaddrinfo qui n'est plus utiliser
    freeaddrinfo(pResult);

    //rendu ici, le client à pu établir la connection avec le serveur, il doit maintenant s'authentifier en lui envoyant son nom d'utilisatueur et son mot de passe
    // afin d'accèder au fichier
    std::stringstream ss;
    ss << argv[2] << "," << argv[3] << "&&";
    string tempoString = ss.str();

    const char* usernamePassword = tempoString.c_str();
    cout << "Identification en cours...\n";

    if (send(connectSocket, usernamePassword, strlen(usernamePassword), 0) == SOCKET_ERROR)
    {
        cout << "Erreur lors de l'authentification!\n";
        closesocket(connectSocket);
        WSACleanup();
    }

    if (recv(connectSocket,recvBuffer,23,0) > 0)
    {
        string tempo1(recvBuffer,23);
        cout <<"\n" << tempo1 << "\n";
    }

    int choix = 0;
    while (choix != 2)
    {
        int fichierATelecharge = 0;
        cout << "Menu" << "\n";
        cout << "1) Afficher la liste de fichier du serveur" << "\n";
        cout << "2) Se deconnecter" << "\n";
        choix = verification();

        //si choix == 1, cela veut dire qu'on envoie une requête au serveur pour voir les fichiers disponible
        if (choix == 1)
        {
            cout << "Envoie une requete pour les fichier" << "\n";
            send(connectSocket, "1&&", 3, 0);
           // memset(recvBuffer, 0, sizeof recvBuffer);

            // recoit le nombre de fichier qui seront envoyer
            recv(connectSocket, recvBuffer, 512, 0);
            string tempo(recvBuffer, strlen(recvBuffer));
            tempo = tempo.substr(0, tempo.find_first_of("&&"));
 
            int nBfichier = stoi(tempo);
            memset(recvBuffer, 0, sizeof recvBuffer);
            // recoit les nom de fichiers disponible
            cout << "Voici une liste de fichier disponible:" << "\n";
            
                
                   recv(connectSocket, recvBuffer, 2048, 0);
                
                    string tempo1(recvBuffer, strlen(recvBuffer));
                    replace(tempo1.begin(), tempo1.end(), '\\', '/');
                    string restantBuffer;
                   // restantBuffer = tempo.substr(tempo.find("&&"+2));

                    for (int i = 0; i < nBfichier; i++)
                    {
                        restantBuffer = tempo1.substr(0, tempo1.find_last_of("&&") + 1);
                        vecteurPath.push_back(restantBuffer.substr(0, restantBuffer.find_first_of("&&")));
                        tempo1 = tempo1.substr(tempo1.find("&&") + 2);
                        cout << vecteurPath.at(i) << "\n";
                       
                    }

                    memset(recvBuffer, 0, sizeof recvBuffer);
             
            cout << "Ecrire le fichier" << "\n";
            while ((fichierATelecharge <= nBfichier && 0 < fichierATelecharge) == false)
            {
            fichierATelecharge = verification();
            }
            tempo = to_string(fichierATelecharge) + "&&";
            //donc on envoie le numero du fichier choisis au server
            send(connectSocket, tempo.c_str(), strlen(tempo.c_str()), 0);
            
            memset(recvBuffer, 0, sizeof recvBuffer);

            // recupère la taille du fichier qui sera envoyer
            recv(connectSocket, recvBuffer, 512, 0);
            string taille(recvBuffer, strlen(recvBuffer));

            bufferPasFixe = new char[stoi(taille)];
            memset(recvBuffer, 0, sizeof recvBuffer);

            
            // recoit le fichier (maximum 1GB) pour l'implentation actuel
            recv(connectSocket, bufferPasFixe, stoi(taille), 0);
            const char* fichierEcrire = bufferPasFixe;

            cout << "Le fichier est en cours d'ecriture..." << "\n";

            tempo = vecteurPath.at(fichierATelecharge-1).substr(vecteurPath.at(fichierATelecharge - 1).rfind("Fichier"), vecteurPath.at(fichierATelecharge - 1).size());

            ofstream file(tempo, ios::out |std::ofstream::binary);
            file.write(fichierEcrire, stoi(taille));
            file.close();
        }
    }

    //deconnection du serveur
    send(connectSocket, "2&&", 3, 0);
    cout << "deconnection reussi!\n";
    closesocket(connectSocket);
    WSACleanup();
    exit(0);
}
