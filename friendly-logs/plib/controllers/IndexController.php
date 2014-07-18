<?php

class IndexController extends pm_Controller_Action
{
    private $checkGroup = null;
    private $logNamesGroup = null;
    public function init()
    {
        parent::init();
        $this->view->pageTitle = 'Example Module';
	// Include js files.
	$scriptURL = pm_Context::getBaseUrl() . 'js/uiScript.js';
	$this->view->headScript()->appendFile($scriptURL);

        // Init tabs for all actions // No needed
      /*  $this->view->tabs = array(
            array(
                'title' => 'Form',
                'action' => 'form',
            ),
	    array(
                'title' => 'List',
                'action' => 'list',
            ),
        );*/
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
                $checkbox->setCheckedValue($one_name);
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

        // Decorator for our display group with checkboxes
        $this->logNamesGroup->setDecorators(array(
            'FormElements', 'Fieldset',
            array('HtmlTag', array('tag'=>'div', 'style'=>'width:30%;height:80vh;float:left;
                                            overflow-y:auto;'))
        ));
    }
    private function buildLogOutputPlace()
    {
        $selectElem = new Zend_Form_Element_Select('logList', array(
                'label'=>'Logs', 'size'=>'2', 
                'style'=>'width:65%;height:80vh;float:right;overflow-y:auto;'));
        $selectElem->addMultiOptions(array('0'=>'some','1'=>'other','2'=>'fuck','3'=>'you', '4'=>'ok?'));
        $this->addElement($selectElem);
    }
}
