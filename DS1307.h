#ifndef DS1307_H
#define DS1307_H

/** Adresse I2C du module RTC DS1307 */
const uint8_t DS1307_ADDRESS = 0x68;

/** Adresse du registre de contrôle du module RTC DS1307 */
const uint8_t DS1307_CTRL_REGISTER = 0x07;

/** Adresse et taille de la NVRAM du module RTC DS1307 */
const uint8_t DS1307_NVRAM_BASE = 0x08;
const uint8_t DS1307_NVRAM_SIZE = 56;


/** Structure contenant les informations de date et heure en provenance ou à destination du module RTC */
typedef struct {
  uint8_t seconds; /**!< Secondes 00 - 59 */
  uint8_t minutes; /**!< Minutes 00 - 59 */
  uint8_t hours;  /**!< Heures 00 - 23 (format 24h), 01 - 12 (format 12h) */
  uint8_t is_pm; /**!< Vaut 1 si l'heure est en format 12h et qu'il est l'aprés midi, sinon 0 */
  uint8_t day_of_week;  /**!< Jour de la semaine 01 - 07, 1 = lundi, 2 = mardi, etc.  */
  uint8_t days; /**!< Jours 01 - 31 */
  uint8_t months;  /**!< Mois 01 - 12 */
  uint8_t year;  /**!< Année au format yy (exemple : 16 = 2016) */
} DateTime_t;


/** Mode de fonctionnement pour la broche SQW */
typedef enum {
  SQW_1_HZ = 0, /**!< Signal à 1Hz sur la broche SQW */
  SQW_4096_HZ,  /**!< Signal à 4096Hz sur la broche SQW */
  SQW_8192_HZ,  /**!< Signal à 8192Hz sur la broche SQW */
  SQW_32768_HZ, /**!< Signal à 32768Hz sur la broche SQW */
  SQW_DC /**!< Broche SQW toujours à LOW ou HIGH */
} DS1307_Mode_t;

#endif /* DS1307_H */