<?php

class IndexController extends pm_Controller_Action
{
    public function init()
    {
        parent::init();
        $this->view->pageTitle = 'Example Module';

        // Init tabs for all actions
        $this->view->tabs = array(
            array(
                'title' => 'Form',
                'action' => 'form',
            ),
        );
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

        // Init form here
        $form = new pm_Form_Simple();
        $form->addElement('text', 'exampleText', array(
            'label' => 'Example Text',
            'value' => pm_Settings::get('exampleText'),
            'required' => true,
            'validators' => array(
                array('NotEmpty', true),
            ),
        ));
		
        $this->view->form = $form;
    }

}
