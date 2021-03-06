<?php

if (!function_exists("tuotteen_alvtarkista")) {
  function tuotteen_alvtarkista(&$t, $i, $result, $tunnus, &$virhe, $trow) {
    global $kukarow, $yhtiorow, $alias_set;
    static $tem_tuoteno, $tem_maa, $chtuoteno_ind;

    if (mysql_field_name($result, $i) == "tuoteno") {

      $t[$i] = trim($t[$i]);

      $query = "SELECT yhtio
                FROM tuote
                WHERE yhtio = '$kukarow[yhtio]'
                and tuoteno = '$t[$i]'";
      $chkressiresult = pupe_query($query);

      if (mysql_num_rows($chkressiresult) == 0) {
        $virhe[$i] = t("Tuotenumero ei l�ydy");
      }

      $tem_tuoteno = $t[$i];
      $chtuoteno_ind = $i;
    }

    if (mysql_field_name($result, $i) == "maa") {
      if ($t[$i] == $yhtiorow["maa"]) {
        $virhe[$i] = t("T�m� on turhaa, sy�t� t�m� alv tuotetietojen taakse.");
      }

      $tem_maa = $t[$i];
    }

    if (mysql_field_name($result, $i) == "alv") {
      if (!is_numeric($t[$i]) or $t[$i] > 100) {
        $virhe[$i] = t("Virheellinen ALV");
      }
    }

    if ((mysql_field_name($result, $i) == "tilino" or
        mysql_field_name($result, $i) == "tilino_eu" or
        mysql_field_name($result, $i) == "tilino_ei_eu" or
        mysql_field_name($result, $i) == "tilino_marginaali" or
        mysql_field_name($result, $i) == "tilino_osto_marginaali") and $t[$i] != 0) {

      $query = "SELECT nimi
                FROM tili
                WHERE yhtio = '$kukarow[yhtio]'
                and tilino  = '$t[$i]'";
      $sresult = pupe_query($query);

      if (mysql_num_rows($sresult) != 1) {
        $virhe[$i] = t("Tili puuttuu tai sit� ei l�ydy");
      }
    }

    if (mysql_field_name($result, $i) == "tunnus") {

      $query = "SELECT tunnus
                FROM tuotteen_alv
                WHERE yhtio  = '$kukarow[yhtio]'
                and maa      = '$tem_maa'
                and tuoteno  = '$tem_tuoteno'
                and tunnus  != '$trow[$i]'";
      $sresult = pupe_query($query);

      if (mysql_num_rows($sresult) > 0) {
        // Pit�� aina setata my�s "t�m�n" kent�n virhe, muuten luedata ei toimi
        $virhe[$chtuoteno_ind] = $virhe[$i] = t("VIRHE: N�ill� tiedoilla on jo alv j�rjestelm�ss�!");
      }
    }
  }
}
