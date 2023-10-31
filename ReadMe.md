# DCS - Server de Comunicare Deschisă

## Introducere

Acesta este un proiect care implementează protocoalele SMTP (Simple Mail Transfer Protocol) și IMAP (Internet Message Access Protocol) în limbajul C pe platforma Ubuntu 22.04. Acest proiect vă permite să creați un server de e-mail capabil să gestioneze trimiterea și primirea de e-mailuri.

## Scopul Proiectului

Scopul principal al acestui proiect este de a oferi o implementare de bază a protocoalelor SMTP și IMAP, pe care o puteți utiliza ca punct de plecare pentru dezvoltarea unui server de e-mail mai complex sau pentru învățarea detaliilor acestor protocoale.

## Caracteristici Cheie

- Implementarea protocolului SMTP pentru trimiterea e-mailurilor către alte servere sau destinatari.
- Implementarea protocolului IMAP pentru accesarea, descărcarea și gestionarea e-mailurilor stocate pe server.
- Gestionarea corectă a erorilor și a conformității cu specificațiile RFC.

## Cum să Utilizați

### Configurare și Compilare

1. Asigurați-vă că aveți instalat un compilator C pe Ubuntu 22.04.
2. Descărcați codul sursă al proiectului de pe acest repository GitHub.
3. Navigați în directorul proiectului și executați comanda de compilare.

### Rulare

1. După compilare, puteți porni serverul cu comanda `./server`.
2. Serverul va începe să asculte conexiuni SMTP și IMAP pe porturile corespunzătoare.
3. După compilare, puteți porni clientul cu comanda `./client`.

### Documentație

Documentația completă a proiectului este disponibilă în [https://github.com/Marian-Irodion/PSO_P-Mail_Server]

## Cerințe

- Ubuntu 22.04 sau o altă distribuție Linux similară.
- Un compilator C (de exemplu, GCC) pentru a compila codul sursă.

