import { AfterViewInit, Component, OnInit, ViewChild } from '@angular/core';
import { NgForm } from '@angular/forms';

import { JobQCommands } from 'src/app/services/interfaces/setup.interface';
import { SetupService } from 'src/app/services/setup.service';

@Component({
  selector: 'app-jobqueue-commands',
  templateUrl: './jobqueue-commands.component.html',
  styleUrls: ['./jobqueue-commands.component.css']
})
export class JobqueueCommandsComponent implements OnInit, AfterViewInit {

  @ViewChild("jobqcommands")
  currentForm!: NgForm;

  JobQCommandsData!: JobQCommands;
  items: number [] = [0,1,2,3];

  constructor(private setupService: SetupService) {
    this.JobQCommandsData = this.setupService.getJobQCommands();
  }

  ngOnInit(): void {
  }

  ngAfterViewInit() {
    this.setupService.setCurrentForm(this.currentForm);
  }

  showHelp() {
    console.log("show help clicked");
    console.log(this);
  }

  saveForm() {
    console.log("save form clicked");
    this.setupService.saveJobQCommands(this.currentForm);
  }

}
