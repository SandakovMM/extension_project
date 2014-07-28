<?php

class IndexController extends pm_Controller_Action
{
    public function init()
    {
        parent::init();
        $this->view->pageTitle = 'Log viewer';
    	// Include js files.
        $extURL = pm_Context::getBaseUrl();
    	$fileURL = $extURL . 'js/uiScript.js?v5';
    	$this->view->headScript()->appendFile($fileURL);
        // Add css files
        $fileURL = $extURL . 'css/outLogsStyle.css';
        $this->view->headLink()->appendStylesheet($fileURL);
        // Check working of log server and start it if needed.
        //exec("echo " . $extURL . " > resfile.txt");
        /*exec("pgrep log-server", $output, $return);
        if ($return != 0) {
            exec($extURL . 'bin/log-server 10020 ' . $extURL . 'bin/pleskcert.crt '
                . $extURL . 'bin/pleskcert.key > ' . $extURL  . 'bin/outfile.txt 2 > /dev/null &'); // Start server here.
        } */// Stream of programm go to dev null now. Maybe later we save it into some file.
     }

    public function indexAction()
    {
        $this->view->test = 'This is index action for testing module.';

        // making redirect if we don't has domain.
        $client = pm_Session::getClient();
        if (($client->isAdmin() || $client->isReseller()) && !pm_Session::isImpersonated()) {
            $this->_forward('overview');
            return 0;
        }

        $form = new ExtensionForm(); // init form class
        $form->build();

        $this->view->form = $form;

    }

    public function overviewAction()
    {
        $this->view->pageTitle = 'Help page';
    }
}

class ExtensionForm extends pm_Form_Simple
{
    private $logPlaceGroup = null;
    private $logNamesGroup = null;

    public function build()
    {
        $this->buildFileNamesGroup();
        $this->buildLogOutputPlace();
        $this->buildConfigPlace();

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
                    $this->addDisplayGroup($logNameArray, 'group'/*, array('legend'=>'Log file names')*/);
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
                // Add button for clear all options from log list.
        $clearButton = $this->createElement('button', 'clearAll', array(
                                            'label' => 'Clear all entryes.'));
        $clearButton->setAttrib('onclick', 'Reaction.clearAll()');
        $this->addElement($clearButton);
        $this->logNamesGroup->addElement($clearButton);

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
        /*$selectElem = new Zend_Form_Element_Select('logList', array(
                'label'=>'Logs', 'size'=>'2', 
                'style'=>'height:70vh;width:100%;overflow:auto;'));

        $this->addElement($selectElem);

        $logPlaceArray[] = $selectElem->getName();
        $this->addDisplayGroup($logPlaceArray, 'logGroup', array());
        $this->logPlaceGroup = $this->getDisplayGroup('logGroup');
        $this->logPlaceGroup->setDecorators(array(
            'FormElements', 'Fieldset',
            array('HtmlTag', array('tag'=>'div', 'style'=>'width:70%;height:80vh;
                                        float:right;overflow-x:auto;overflow-y:auto;')),
	    array(array('pages'=>'HtmlTag'), array('tag'=>'div', 'id'=>'pages',
	    	'style'=>'width:70%;height:50px;overflow:auto','placement'=>'append'))
        ));*/
	$this->addElement('hidden', 'outputDive',array(
		'required' => false,
		'ignore' => true,
		'autoInsertNotEmptyValidator' => false,
		'decorators' => array(
			array( array( 'divLogList' => 'HtmlTag' ), 
				array( 'tag' => 'div',
					'id' => 'logList',
					'style'=>'height:70vh; width:70%; overflow:auto;',
					'placement' => 'append' 
					) 
			),
			array( array( 'divPages' => 'HtmlTag' ),
				array( 'tag' => 'div',
				'id' => 'pages',
				'style'=>'height:50px; width:70%; overflow:auto;margin: inherit inherit inherit 30%',
				'placement' => 'append'
				)
			)
		)
	));
    }

    private function buildConfigPlace()
    {
        // Checkbox for some settings.
        $checkbox = $this->createElement('checkbox', 'sorting',
                         array('label' => 'Use time sorting for old entryes.'));
        $this->addElement($checkbox);

        $configs[] = $checkbox->getName();
        $this->addDisplayGroup($configs, 'configGroup', array());
        $configGroup = $this->getDisplayGroup('configGroup');
        $configGroup->setDecorators(array(
            'FormElements', 'Fieldset',
            array('HtmlTag', array('tag'=>'div', 'title'=>'See me?',
                'style'=>'float:left;overflow-x:auto;overflow-y:auto;'))
            ));/**/
    }
}
