<?php
/**
 * Kirjasto PDF eKirjeen l�hetykseen
 *
 * Luo Itellan vaatiman XML filen jossa kirjeen tiedot. L�hetett�v�
 * tiedosto (PDF) sek� XML file pistet��n ZIP fileen joka l�hetet��n
 * FTP:ll� Itellaan.
 *
 * Esim:
 *
 * <code>
 * $ekirje = new Pupe_Pdfekirje($info);
 *
 * try {
 *     $ekirje->send('/tmp/pdf_file.pdf');
 * } catch(Exception $e) {
 *     echo $e->getMessage();
 * }
 * </code>
 *
 * Tarvitaan salasanat.php config:
 *
 * $ekirje_config = array(
 *     'ftp_host'              => 'tuljuftp.enet.fi',  // ftp palvelimen osoite
 *     'ftp_user'              => 'username',      // ftp palvelimen username
 *     'ftp_pass'              => 'password',      // ftp palvelimen passwd
 *     'ftp_path'              => 'PDFekirje/',      // vakiopolku ftp-palvelimella
 *     'is_test'               => false,        // true/false l�hetet��nk� kirje postitse asiakkaalle asti
 *     'asiakas_tunnus'        => 'userid',        // asiakkaan itella tunnus
 *     'salasana'              => 'passwd',        // asiakkaan itella passwd
 * );
 *
 * @author Antti Holvikari <antth@gmail.com>
 *
 */


class Pupe_Pdfekirje {

  /**
   * eKirjeeseen tarvittavat tiedot
   *
   * @var array
   *
   */
  protected $_info = array(
    'ftp_host'              => 'tuljuftp.enet.fi',
    'ftp_user'              => 'USER',
    'ftp_pass'              => 'PASS',
    'ftp_path'              => '/path/to/dir/',
    'tunniste'              => '',              // asiakkaan oma kirjeen tunniste
    'is_test'               => 'true',          // 'true' tai 'false', ei PHP boolean arvo
    'asiakas_tunnus'        => null,            // asiakkaan itella tunnus
    'salasana'              => 'XXXX',
    'kirjeluokka'           => '1',             // 1 = priority, 2 = economy
    'osasto'                => '',              // osastokohtainen erittely
    'file_id'               => null,            // l�hett�j�n tunniste tiedostolle
    'kirje_id'              => null,            // kirjeen id
    'contact_name'          => null,
    'contact_email'         => null,
    'contact_phone'         => null,
    'yritys_nimi'           => null,
    'yritys_osoite'         => null,
    'yritys_postino'        => null,
    'yritys_postitp'        => null,
    'yritys_maa'            => null,
    'vastaanottaja_nimi'    => null,
    'vastaanottaja_osoite'  => null,
    'vastaanottaja_postino' => null,
    'vastaanottaja_postitp' => null,
    'vastaanottaja_maa'     => null,
    'sivumaara'             => '1',
    'tmp_dir'               => '/tmp',
  );

  /**
   * ZIP filen nimi
   *
   * @var string
   *
   */
  private $_zip_filename;

  /**
   * PDF tiedoston nimi ZIP tiedoston sis�ll�
   *
   * @var string
   *
   */
  private $_pdf_filename;

  /**
   * Constructori
   *
   * Yli-kirjoitetaan defaultit
   *
   * @return void
   *
   */
  public function __construct($config = array()) {
    $this->_info = array_merge($this->_info, (array) $config);

    // jos viimeinen merkki pathiss� ei ole kauttaviiva lis�t��n kauttaviiva...
    if (substr($this->_info['ftp_path'], -1) != "/") {
      $this->_info['ftp_path'] .= '/';
    }

    if (substr($this->_info['tmp_dir'], -1) != "/") {
      $this->_info['tmp_dir'] .= '/';
    }

    // is_test pit�� olla (string) 'true' tai 'false'
    $this->_info['is_test'] = ($this->_info['is_test'] == true) ? 'true' : 'false';
  }

  /**
   * L�hett�� filen FTP:n avulla
   *
   * @return void
   *
   */
  public function send($file) {
    if (! is_readable($file)) {
      throw new Exception("File $file does not exist or is not readable.");
    }

    $pdf_file = $this->_info['tmp_dir'] . 'ekirje-' . $this->_info['kirje_id'] . '.pdf';

    if (! copy($file, $pdf_file)) {
      throw new Exception(
        'Cannot copy PDF file.',
        array('pdf_filename' => $pdf_file)
      );
    }

    // asetetaan pdf tiedoston nimi
    $this->_pdf_filename = $pdf_file;

    // tehd��n XML file
    $this->_generateXmlFile();

    // tehd��n zip jossa XML file sek� PDF file
    $this->_zip();

    // l�hetet��n FTP:ll� zippi
    $this->_ftpSend();
  }

  /**
   * Poistaa temp filet koneelta
   *
   * @return void
   *
   */
  public function clean() {
    unlink($this->_pdf_filename);
    unlink($this->_xml_filename);
    unlink($this->_zip_filename);
  }

  /**
   * Tekee XML datan
   *
   * @return string Polku XML fileen
   *
   */
  protected function _generateXmlFile() {
    // haetaan XML template
    $xml = $this->_getXml();

    $o = array(
      // tiedoston nimi zipin sis�ll�
      'file_nimi' => basename($this->_pdf_filename),
    );

    // lis�t��n PDF filen nimi joka on ZIPin sis�ll�
    $info = array_merge($this->_info, $o);

    // korvataan placeholderit oikeilla tiedoilla
    foreach ($info as $key => $val) {
      $val = str_replace(
        array('<', '>', '"', "'", '&'),
        array(
          htmlentities('<'),
          htmlentities('>'),
          htmlentities('"', ENT_QUOTES),
          htmlentities("'", ENT_QUOTES),
          htmlentities('&'),
        ),
        $val
      );

      $xml = str_replace(":$key", $val, $xml);
    }

    $file = $this->_info['tmp_dir'] . 'ekirje-' . $this->_info['kirje_id'] . '.xml';

    // kirjoitetaan xml tiedostoon
    if (! file_put_contents($file, $xml)) {
      throw new Exception('Could not create XML file.');
    }

    $this->_xml_filename = $file;

    // palautetaan xml file nimi joka juuri tehtiin
    return $this->_xml_filename;
  }

  /**
   * Tekee ZIPin jossa xml- sek� pdf file
   *
   * @return string Zip filen polku
   *
   */
  protected function _zip() {
    $zipfile = $this->_info['tmp_dir'] . 'ekirje-' . $this->_info['kirje_id'] . '.zip';
    $command = 'zip -j ' . escapeshellcmd($zipfile)
      . ' ' . escapeshellcmd($this->_xml_filename)
      . ' ' . escapeshellcmd($this->_pdf_filename);

    $out = exec($command);

    // asetetaan zip filenimi
    $this->_zip_filename = $zipfile;

    // palautetaan my�s
    return $this->_zip_filename;
  }

  /**
   * L�hett�� itse zip tiedoston FTP:ll�
   *
   * @return bool true
   *
   */
  protected function _ftpSend() {
    if (empty($this->_info['ftp_host'])
      || empty($this->_info['ftp_user'])
      || empty($this->_info['ftp_pass'])
      || empty($this->_info['ftp_path'])
    ) {
      throw new Exception("FTP info missing.");
    }

    $conn_id = @ftp_connect($this->_info['ftp_host']);

    if (! $conn_id) {
      throw new Exception("Could not connect to remote host {$this->_info['ftp_host']}");
    }

    // jos connectio ok, kokeillaan loginata
    $login = @ftp_login(
      $conn_id,
      $this->_info['ftp_user'],
      $this->_info['ftp_pass']
    );

    if (! $login) {
      throw new Exception("Could not log in.");
    }

    // jos login ok kokeillaan uploadata
    $upload = @ftp_put(
      $conn_id,
      $this->_info['ftp_path'] . basename($this->_zip_filename),
      realpath($this->_zip_filename),
      FTP_BINARY
    );

    if (! $upload) {
      throw new Exception("Could not upload to directory {$this->_info['ftp_path']}");
    }

    // disconnect
    if ($conn_id) {
      ftp_close($conn_id);
    }

    return true;
  }

  /**
   * Palauttaa XML templaten
   *
   * @return string eKirje XML
   *
   */
  private function _getXml() {
    return <<<XML
<?xml version="1.0" encoding="ISO-8859-15"?>
<lb:LetterBundle xmlns:lb="urn:itella-letterbundle-v0x3" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="urn:itella-letterbundle-v0x3 LetterBundle_V0x3.xsd" lb:schemaVersion="v0x3">
  <lb:Bundle lb:senderBundleId=":tunniste">
    <lb:BundleCommon lb:isTest=":is_test">
      <lb:Sender lb:contact=":contact_name, :contact_email, :contact_phone">
        <lb:SenderAddress>
          <lb:Eu1 lb:name=":yritys_nimi" lb:address=":yritys_osoite" lb:postalCode=":yritys_postino" lb:city=":yritys_postitp" lb:countryCode=":yritys_maa"/>
        </lb:SenderAddress>
      </lb:Sender>
      <lb:StdBundleProcessing lb:customerId=":asiakas_tunnus" lb:password=":salasana" lb:serviceFuntion="0" lb:electronicArchiving="T" lb:letterClass=":kirjeluokka" lb:envelope="S" lb:paper="0" lb:fileFormat="0" lb:departmentClassification="true" lb:isDuplex="false"/>
      <lb:ExternalFile lb:senderFileId=":file_id" lb:format="PDF" lb:fileName=":file_nimi"/>
    </lb:BundleCommon>
    <lb:Letter lb:senderLetterId=":kirje_id">
      <lb:LetterMeta>
        <lb:StdLetterProcessing lb:departmentClassificationCode=":osasto"/>
        <lb:ReceiverAddress>
          <lb:Eu1 lb:name=":vastaanottaja_nimi" lb:address=":vastaanottaja_osoite" lb:postalCode=":vastaanottaja_postino" lb:city=":vastaanottaja_postitp" lb:countryCode=":vastaanottaja_maa"/>
        </lb:ReceiverAddress>
        <lb:Location lb:startPage="1" lb:pages=":sivumaara"/>
      </lb:LetterMeta>
    </lb:Letter>
  </lb:Bundle>
</lb:LetterBundle>
XML;

  }
}
