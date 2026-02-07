# Definir Jeton GITHUB

## Auteur : Patrice Waechter-Ebling
* Programme Win32 pour définir une variable d’environnement GITHUB_TOKEN
* Utilisation d'un template de dialog au lieu du traditionel dialog dans le RC
* avec un jeton d’accès personnel GitHub (PAT) stocké de manière sécurisée
* dans le registre Windows en utilisant DPAPI pour le chiffrement.
* Fonctionnalités :
    dialogue Win32 (Edit + bouton)
• 	stockage chiffré avec DPAPI dans le registre
• 	lecture/déchiffrement
• 	mise à jour de  dans l’environnement du processus
