<?php

class IndexController extends pm_Controller_Action
{
    private $checkGroup = null;
    private $logNamesGroup = null;
    public function init()
    {
        parent::init();
        $this->view->pageTitle = 'Log viewer';
    	// Include js files.
        $extURL = pm_Context::getBaseUrl();
    	$fileURL = $extURL . 'js/uiScript.js?v5';
    	$this->view->headScript()->appendFile($fileURL);
        // Add css files
        $fileURL = $extURL . 'css/putLogsStyle.css';
        $this->view->headLink()->appendStylesheet($fileURL);
        // Check working of log server and start it if needed.
        exec("pgrep log-server", $output, $return);
        if ($return != 0) {
            exec('/home/smm/projects/plesk/extension_project/log-server/log-server 127.0.1.1 10010 > /dev/null 2 > /dev/null &'); // Start server here.
        } // Stream of programm go to dev null now. Maybe later we save it into some file.
     }

    public function indexAction()
    {
        // Default action will be formAction
		$this->_forward('form');
    }

    public function formAction()
    {
        // Display simple text in view
        $this->view->test = 'This is index action for testing module.';

        $form = new ExtensionForm(); // init form class
        $form->build();

        $this->view->form = $form;

    }
}

class ExtensionForm extends pm_Form_Simple
{
    private $checkGroup = null;
    private $logNamesGroup = null;

    public function build()
    {
        $this->buildFileNamesGroup();
        $this->buildLogOutputPlace();
    }

    private function buildFileNamesGroup() // add checks with log file name to a form
    {
        $this->logNamesGroup = $this->getDisplayGroup('group');
        // Geting domain name what we work.
        $domain = pm_Session::getCurrentDomain();
        $domainName = $domain->getName();
        
        // get needed names with shell command ls
        $logs_names = scandir/*glob*/('/var/www/vhosts/system/' .
            $domainName . '/logs');
        $logNameArray = array();
        
        // Make display group with log file names
        foreach ($logs_names as $one_name) {
            if (!empty($one_name) && '.' != $one_name && '..' != $one_name) {
                $counter = $counter + 1;    // Make element
                $checkbox = $this->createElement('checkbox', 'log' . $counter,
                         array('label' => $one_name));
                $checkbox->setCheckedValue($one_name); // Like that now, change later maybe
                $checkbox->setAttrib('onclick', 'Reaction.clickFileCheck(this)');
                $this->addElement($checkbox);
                    // Add element to group
                if (is_null($this->logNamesGroup)) {
                    $logNameArray[] = $checkbox->getName();
                    $this->addDisplayGroup($logNameArray, 'group', array('legend'=>'Log file names'));
                    $this->logNamesGroup = $this->getDisplayGroup('group');
                }
                $this->logNamesGroup->addElement($checkbox);/* Find the way how do it normal*/          
            }
        }

        // Last button, who must set all others seted or somthing like this
        $checkbox = $this->createElement('checkbox', 'setupAll', array(
                'label' => 'Total count is:' . $counter));
        $checkbox->setCheckedValue($counter);
        $checkbox->setAttrib('onclick', 'Reaction.clickAllFiles(this)');
        $this->addElement($checkbox);
        $this->logNamesGroup->addElement($checkbox);/* Find the way how do it normal*/

                                // Add element with nedded for server information.
        $serverIP = $_SERVER['SERVER_ADDR'];
        //$serverIP = gethostbyname(gethostname()); // second version to get ip adress.
        $hidden = $this->createElement('hidden', 'someInformation');
        $hidden->setValue($domainName . ' ' . $serverIP);
        //$hidden->setAttrib('onload', 'SocketWorker(ws://' . $serverIP . ':20030)');
        $this->addElement($hidden);

        // Decorator for our display group with checkboxes
        $this->logNamesGroup->setDecorators(array(
            'FormElements', 'Fieldset',
            array('HtmlTag', array('tag'=>'div', 'style'=>'width:30%;height:80vh;
                                                        float:left;overflow-y:auto;'))
        ));
    }
    private function buildLogOutputPlace()
    {
        $selectElem = new Zend_Form_Element_Select('logList', array(
                'label'=>'Logs', 'size'=>'2', 
                'style'=>'width:65%;height:80vh;float:right;overflow-y:auto;'));
        $this->addElement($selectElem);
    }
}
