# SaproFDS2024

Pour l'installation **"De temps en temps"**, commande du **Forum des Sciences**, réalisée par les **Saprophytes** et **metalu.net**.

## Programmation du RaspberryPi Pico (français):

- ### Installer PureData 
[voir ici](https://puredata.info/downloads/pure-data).

- ### Installer la bibliothèque d'objets PureData [Fraise](https://github.com/MetaluNet/Fraise) :

    - lancer PureData (Pd)
    - ouvrir le menu *Outils/Installer des objets supplémentaires* (ou *Tools/Find externals*)
    - chercher "Fraise"
    - installer la bibliothèque **Fraise**
    - optionnel : si on veut modifier le *firmware* (programme du Pico), installer aussi la bibliothèque **Fraise-toolchain**
    - pour Linux, il faut autoriser l'utilisateur à ouvrir les ports séries, en tapant dans un terminal :  
     `sudo adduser [nom_utilisateur] dialout`

- ### Installer le *bootloader* Fraise sur le Pico :
    - trouver l'emplacement du dossier où Fraise est installé (dépend de l'OS et de la configuration de Pd; est indiqué dans les préférences de la fenêtre d'installation des bibliothèques)
    - connecter le Pico en USB en mode BOOT : maintenir appuyé le bouton BOOTSEL du Pico pendant la connexion du câble USB ; l'ordinateur doit détecter un nouveau volume USB
    - copier le fichier `Fraise/boards/pico/usb_bootloader.uf2` dans ce nouveau volume ; le Pico doit redémarrer et se mettre à clignoter

- ### Télécharger le projet SaproFDS2024 dans l'ordinateur
    - télécharger l'[archive du projet](https://github.com/MetaluNetProjects/SaproFDS2024/archive/refs/heads/main.zip)
    - la décompresser (l'extraire) quelque part

- ### Installer et configurer le *firmware* du projet
    - lancer Pd
    - ouvrir le *patch* correspondant au projet:
        - `SaproFDS2024/jour_nuit/0journuit.pd`  
        <u>ou</u>
        - `SaproFDS2024/chrono/0chrono.pd`  
        (ne pas les ouvrir tous les deux en même temps)
    - brancher le Pico sur l'ordi ; les objets `pied/pied` et `fruit/fruit` doivent s'allumer en jaune.
    - cliquer sur le bouton `utils` de l'objet `fruit/fruit` ; une nouvelle fenêtre "Fraise: fruit utils" doit s'ouvrir.
    - sur cette fenêtre, cliquer sur le bouton `PROGRAM` (jaune), attendre la fin du chargement ; fermer la fenêtre `utils`
    - une fois le chargement terminé, le Pico redémarre et les objets `pied` et `fruit` se rallument ; cliquer alors sur le bouton rouge `SAVE_SETTINGS` sur patch principal.

Me contacter en cas de problème ;-)

------------


Antoine Rousseau @ metalu.net 2024

